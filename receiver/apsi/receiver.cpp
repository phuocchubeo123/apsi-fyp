// STD
#include <algorithm>
#include <future>
#include <iostream>
#include <sstream>
#include <stdexcept>

// APSI
#include "apsi/log.h"
#include "apsi/network/channel.h"
#include "apsi/receiver.h"
#include "apsi/thread_pool_mgr.h"
#include "apsi/util/label_encryptor.h"
#include "apsi/util/utils.h"

// SEAL
#include "seal/ciphertext.h"
#include "seal/context.h"
#include "seal/encryptionparams.h"
#include "seal/keygenerator.h"
#include "seal/plaintext.h"
#include "seal/util/common.h"
#include "seal/util/defines.h"

using namespace std;
using namespace seal;
using namespace seal::util;
using namespace kuku;

namespace apsi {
    using namespace util;
    using namespace network;

    namespace {
        template <typename T>
        bool has_n_zeros(T *ptr, size_t count)
        {
            return all_of(ptr, ptr + count, [](auto a) { return a == T(0); });
        }

        template <typename T>
        bool has_n_ones(T *ptr, size_t count)
        {
            return all_of(ptr, ptr + count, [](auto a) { return a == T(1); });
        }
    } // namespace

    namespace receiver {
        size_t IndexTranslationTable::find_item_idx(size_t table_idx) const noexcept
        {
            auto item_idx = table_idx_to_item_idx_.find(table_idx);
            if (item_idx == table_idx_to_item_idx_.cend()) {
                return 1e9;
            }

            return item_idx->second;
        }

        Receiver::Receiver(PSIParams params) : params_(move(params))
        {
            initialize();
        }

        unique_ptr<SenderOperation> Receiver::CreateParamsRequest()
        {
            auto sop = make_unique<SenderOperationParms>();
            APSI_LOG_INFO("Created parameter request");

            return sop;
        }

        PSIParams Receiver::RequestParams(NetworkChannel &chl)
        {
            // Create parameter request and send to Sender
            chl.send(CreateParamsRequest());
            APSI_LOG_INFO("Sent parameter request!")

            // Wait for a valid message of the right type
            ParamsResponse response;
            bool logged_waiting = false;

            APSI_LOG_INFO("Waiting...")

            while (!(response = to_params_response(chl.receive_response()))) {
                if (!logged_waiting) {
                    // We want to log 'Waiting' only once, even if we have to wait for several
                    // sleeps.
                    logged_waiting = true;
                    APSI_LOG_INFO("Waiting for response to parameter request");
                }

                this_thread::sleep_for(50ms);
            }

            APSI_LOG_INFO("Received parameter response!")

            return *response->params;
        }

        void Receiver::initialize()
        {
            APSI_LOG_INFO("PSI parameters set to: " << params_.to_string());

            STOPWATCH(recv_stopwatch, "Receiver::initialize");

            // Initialize the CryptoContext with a new SEALContext
            crypto_context_ = CryptoContext(params_);

            // Create new keys
            reset_keys();
        }

        void Receiver::reset_keys()
        {
            // Generate new keys
            KeyGenerator generator(*get_seal_context());

            // Set the symmetric key, encryptor, and decryptor
            crypto_context_.set_secret(generator.secret_key());

            // Create Serializable<RelinKeys> and move to relin_keys_ for storage
            relin_keys_.clear();
            if (get_seal_context()->using_keyswitching()) {
                Serializable<RelinKeys> relin_keys(generator.create_relin_keys());
                relin_keys_.set(move(relin_keys));
            }
        }

        vector<MatchRecord> Receiver::request_query(
            const vector<Item> &items,
            NetworkChannel &chl)
        {
            ThreadPoolMgr tpm;

            // Create query and send to Sender
            auto query = create_query(items);
            chl.send(move(query.first));
            auto itt = move(query.second);

            // Wait for query response
            QueryResponse response;
            bool logged_waiting = false;

            while (!(response = to_query_response(chl.receive_response()))) {
                if (!logged_waiting) {
                    // We want to log 'Waiting' only once, even if we have to wait for several
                    // sleeps.
                    logged_waiting = true;
                    APSI_LOG_INFO("Waiting for response to query request");
                }

                this_thread::sleep_for(50ms);
            }

            // Set up the result
            vector<MatchRecord> mrs(query.second.item_count());

            // Get the number of ResultPackages we expect to receive
            atomic<uint32_t> package_count{ response->package_count };

            // Launch threads to receive ResultPackages and decrypt results
            size_t task_count = min<size_t>(ThreadPoolMgr::GetThreadCount(), package_count);
            vector<future<void>> futures(task_count);
            APSI_LOG_INFO(
                "Launching " << task_count << " result worker tasks to handle " << package_count
                             << " result parts");

            for (size_t t = 0; t < task_count; t++) {
                futures[t] = tpm.thread_pool().enqueue(
                    [&]() { process_result_worker(package_count, mrs, itt, chl); });
            }

            for (auto &f : futures) {
                f.get();
            }

            APSI_LOG_INFO(
                "Found " << accumulate(mrs.begin(), mrs.end(), 0, [](auto acc, auto &curr) {
                    return acc + curr.found;
                }) << " matches");

            return mrs;
        }

        pair<Request, IndexTranslationTable> Receiver::create_query(const vector<Item> &items)
        {
            APSI_LOG_INFO("Creating encrypted query for " << items.size() << " items");
            STOPWATCH(recv_stopwatch, "Receiver::create_query");

            IndexTranslationTable itt;
            itt.item_count_ = items.size();

            // Create the cuckoo table
            KukuTable cuckoo(
                params_.table_params().table_size,      // Size of the hash table
                0,                                      // Not using a stash
                params_.table_params().hash_func_count, // Number of hash functions
                { 0, 0 },                               // Hardcoded { 0, 0 } as the seed
                cuckoo_table_insert_attempts,           // The number of insertion attempts
                { 0, 0 });                              // The empty element can be set to anything
            {
                STOPWATCH(recv_stopwatch, "Receiver::create_query::cuckoo_hashing");
                APSI_LOG_INFO(
                    "Inserting " << items.size() << " items into cuckoo table of size "
                                 << cuckoo.table_size() << " with " << cuckoo.loc_func_count()
                                 << " hash functions");

                for (size_t item_idx = 0; item_idx < items.size(); item_idx++) {
                    const auto &item = items[item_idx];
                    if (!cuckoo.insert(item.get_as<kuku::item_type>().front())) {
                        // Insertion can fail for two reasons:
                        //
                        //     (1) The item was already in the table, in which case the "leftover
                        //     item" is empty; (2) Cuckoo hashing failed due to too small table or
                        //     too few hash functions.
                        //
                        // In case (1) simply move on to the next item and log this issue. Case (2)
                        // is a critical issue so we throw and exception.
                        if (cuckoo.is_empty_item(cuckoo.leftover_item())) {
                            APSI_LOG_INFO(
                                "Skipping repeated insertion of items["
                                << item_idx << "]: " << item.to_string());
                        } else {
                            APSI_LOG_ERROR(
                                "Failed to insert items["
                                << item_idx << "]: " << item.to_string()
                                << "; cuckoo table fill-rate: " << cuckoo.fill_rate());
                            throw runtime_error("failed to insert item into cuckoo table");
                        }
                    }
                }
                APSI_LOG_INFO(
                    "Finished inserting items with "
                    << cuckoo.loc_func_count()
                    << " hash functions; cuckoo table fill-rate: " << cuckoo.fill_rate());
            }

            // Once the table is filled, fill the table_idx_to_item_idx map
            for (size_t item_idx = 0; item_idx < items.size(); item_idx++) {
                auto item_loc = cuckoo.query(items[item_idx].get_as<kuku::item_type>().front());
                itt.table_idx_to_item_idx_[item_loc.location()] = item_idx;
            }

            APSI_LOG_INFO("Tables filled!");
            vector<PlaintextBits> plaintext_bits;

            string zz(params_.item_params().item_bit_count / 8, 'z');
            Item all_z(zz);

            {
                STOPWATCH(recv_stopwatch, "Receiver::create_query::prepare_data");
                for (uint32_t bundle_idx = 0; bundle_idx < params_.bundle_idx_count();
                     bundle_idx++) {
                    vector<Item> bundle_items;
                    for (int bin_idx = 0; bin_idx < params_.receiver_bins_per_bundle(); bin_idx++){
                        size_t item_idx = itt.find_item_idx(bundle_idx * params_.receiver_bins_per_bundle() + bin_idx);
                        if (item_idx < 1e9) bundle_items.push_back(items[item_idx]);
                        else bundle_items.push_back(all_z);
                    }
                    APSI_LOG_DEBUG("For bundle " << bundle_idx << ", there are " << bundle_items.size() << " items");
                    for (int item_idx = 0; item_idx < bundle_items.size(); item_idx++){
                        APSI_LOG_DEBUG("Token and item: " << bundle_items[item_idx].raw_val() << " " << bundle_items[item_idx].to_string());
                    }
                    plaintext_bits.emplace_back(move(bundle_items), params_);

                    // APSI_LOG_INFO("Bundle: " << bundle_idx << ", Plaintext: ");
                    // APSI_LOG_INFO(plaintext_bits.back().bit_list());
                }
            }

            unordered_map<uint32_t, vector<SEALObject<Ciphertext>>> encrypted_bits;
            // encrypt_data
            {
                STOPWATCH(recv_stopwatch, "Receiver::create_query::encrypt_data");
                for (uint32_t bundle_idx = 0; bundle_idx < params_.bundle_idx_count();
                     bundle_idx++) {
                    APSI_LOG_INFO("Encoding and encrypting data for bundle index " << bundle_idx);

                    // Encrypt the data for this power
                    auto encrypted_bit(plaintext_bits[bundle_idx].encrypt(crypto_context_));

                    // Move the encrypted data to encrypted_powers
                    for (auto &e: encrypted_bit){
                        encrypted_bits[e.first].emplace_back(move(e.second));
                    }
                }
            }

            // Set up the return value
            auto sop_query = make_unique<SenderOperationQuery>();
            sop_query->compr_mode = Serialization::compr_mode_default;
            sop_query->relin_keys = relin_keys_;
            sop_query->data = move(encrypted_bits);
            auto sop = to_request(move(sop_query));

            APSI_LOG_INFO("Finished creating encrypted query");

            return { move(sop), itt };
        }

        void Receiver::process_result_worker(
            atomic<uint32_t> &package_count,
            vector<MatchRecord> &mrs,
            const IndexTranslationTable &itt,
            Channel &chl) const
        {
            stringstream sw_ss;
            sw_ss << "Receiver::process_result_worker [" << this_thread::get_id() << "]";
            STOPWATCH(recv_stopwatch, sw_ss.str());

            APSI_LOG_INFO("Result worker [" << this_thread::get_id() << "]: starting");

            auto seal_context = get_seal_context();

            while (true) {
                // Return if all packages have been claimed
                uint32_t curr_package_count = package_count;
                if (curr_package_count == 0) {
                    APSI_LOG_INFO(
                        "Result worker [" << this_thread::get_id()
                                          << "]: all packages claimed; exiting");
                    return;
                }

                // If there has been no change to package_count, then decrement atomically
                if (!package_count.compare_exchange_strong(
                        curr_package_count, curr_package_count - 1)) {
                    continue;
                }

                // Wait for a valid ResultPart
                ResultPart result_part;
                while (!(result_part = chl.receive_result(seal_context)))
                    ;

                APSI_LOG_INFO(
                    "Result worker [" << this_thread::get_id()
                                        << "]: received one package!");

                // Process the ResultPart to get the corresponding vector of MatchRecords
                auto this_mrs = process_result_part(itt, result_part);

                // Merge the new MatchRecords with mrs
                seal_for_each_n(iter(mrs, this_mrs, size_t(0)), mrs.size(), [](auto &&I) {
                    if (get<1>(I) && !get<0>(I)) {
                        // This match needs to be merged into mrs
                        get<0>(I) = move(get<1>(I));
                    } else if (get<1>(I) && get<0>(I)) {
                        // If a positive MatchRecord is already present, then something is seriously
                        // wrong
                        APSI_LOG_ERROR(
                            "Result worker [" << this_thread::get_id()
                                              << "]: found a match for items[" << get<2>(I)
                                              << "] but an existing match for this location was "
                                                 "already found before from a different result "
                                                 "part");

                        throw runtime_error(
                            "found a duplicate positive match; something is seriously wrong");
                    }
                });
            }
        }

        vector<MatchRecord> Receiver::process_result_part(
            const IndexTranslationTable &itt,
            const ResultPart &result_part) const
        {
            STOPWATCH(recv_stopwatch, "Receiver::process_result_part");

            if (!result_part) {
                APSI_LOG_ERROR("Failed to process result: result_part is null");
                return {};
            }

            // The number of items that were submitted in the query
            size_t item_count = itt.item_count();

            // Decrypt and decode the result; the result vector will have full batch size
            PlainResultPackage plain_rp = result_part->extract(crypto_context_);

            size_t items_per_bundle = safe_cast<size_t>(params_.receiver_bins_per_bundle());
            size_t bundle_start =
                mul_safe(safe_cast<size_t>(plain_rp.bundle_idx), items_per_bundle);

            // Set up the result vector
            vector<MatchRecord> mrs(item_count);

            std::cout << "Results: ";
            for (int i = 0; i < 10; i++){
                std::cout << plain_rp.psi_result[i] << " ";
            }
            std::cout << "\n";

            // Iterate over the decoded data to find consecutive zeros indicating a match
            StrideIter<const uint64_t *> plain_rp_iter(plain_rp.psi_result.data(), 1);
            seal_for_each_n(iter(plain_rp_iter, size_t(0)), items_per_bundle, [&](auto &&I) {
                // Find felts_per_item consecutive ones
                bool match = has_n_ones(get<0>(I).ptr(), 1);
                if (!match) {
                    return;
                }

                // Compute the cuckoo table index for this item. Then find the corresponding index
                // in the input items vector so we know where to place the result.
                size_t table_idx = add_safe(get<1>(I), bundle_start);
                auto item_idx = itt.find_item_idx(table_idx);

                // If this table_idx doesn't match any item_idx, ignore the result no matter what it
                // is
                if (item_idx == itt.item_count()) {
                    return;
                }

                // If a positive MatchRecord is already present, then something is seriously wrong
                if (mrs[item_idx]) {
                    APSI_LOG_ERROR("The table index -> item index translation table indicated a "
                                   "location that was already filled by another match from this "
                                   "result package; the translation table (query) has probably "
                                   "been corrupted");

                    throw runtime_error(
                        "found a duplicate positive match; something is seriously wrong");
                }

                APSI_LOG_DEBUG(
                    "Match found for items[" << item_idx << "] at cuckoo table index "
                                             << table_idx);

                // Create a new MatchRecord
                MatchRecord mr;
                mr.found = true;

                // We are done with the MatchRecord, so add it to the mrs vector
                mrs[item_idx] = move(mr);
            });

            return mrs;
        }

        vector<MatchRecord> Receiver::process_result(
            const IndexTranslationTable &itt,
            const vector<ResultPart> &result) const
        {
            APSI_LOG_INFO("Processing " << result.size() << " result parts");
            STOPWATCH(recv_stopwatch, "Receiver::process_result");

            vector<MatchRecord> mrs(itt.item_count());
        }
    }
}