// STD
#include <future>
#include <sstream>

// APSI
#include "apsi/crypto_context.h"
#include "apsi/log.h"
#include "apsi/network/channel.h"
#include "apsi/network/result_package.h"
#include "apsi/psi_params.h"
#include "apsi/seal_object.h"
#include "apsi/sender.h"
#include "apsi/thread_pool_mgr.h"
#include "apsi/util/stopwatch.h"
#include "apsi/util/utils.h"

// SEAL
#include "seal/evaluator.h"
#include "seal/modulus.h"
#include "seal/util/common.h"

using namespace std;
using namespace seal;
using namespace seal::util;

namespace apsi {
    using namespace util;
    using namespace oprf;
    using namespace network;

    namespace sender {
        void Sender::RunParams(
            const ParamsRequest &params_request,
            shared_ptr<SenderDB> sender_db,
            network::Channel &chl,
            function<void(Channel &, Response)> send_fun)
        {
            STOPWATCH(sender_stopwatch, "Sender::RunParams");

            if (!params_request) {
                APSI_LOG_ERROR("Failed to process parameter request: request is invalid");
                throw invalid_argument("request is invalid");
            }

            // Check that the database is set
            if (!sender_db) {
                throw logic_error("SenderDB is not set");
            }

            APSI_LOG_INFO("Start processing parameter request");

            ParamsResponse response_params = make_unique<ParamsResponse::element_type>();
            response_params->params = make_unique<PSIParams>(sender_db->get_params());

            try {
                send_fun(chl, move(response_params));
            } catch (const exception &ex) {
                APSI_LOG_ERROR(
                    "Failed to send response to parameter request; function threw an exception: "
                    << ex.what());
                throw;
            }

            APSI_LOG_INFO("Finished processing parameter request");
        }

        void Sender::RunQuery(
            const Query &query,
            Channel &chl,
            function<void(Channel &, Response)> send_fun,
            function<void(Channel &, ResultPart)> send_rp_fun)
        {
            if (!query) {
                APSI_LOG_ERROR("Failed to process query request: query is invalid");
                throw invalid_argument("query is invalid");
            }

            // We use a custom SEAL memory that is freed after the query is done
            auto pool = MemoryManager::GetPool(mm_force_new);

            ThreadPoolMgr tpm;

            // Acquire read lock on SenderDB
            auto sender_db = query.sender_db();
            auto sender_db_lock = sender_db->get_reader_lock();

            STOPWATCH(sender_stopwatch, "Sender::RunQuery");
            APSI_LOG_INFO(
                "Start processing query request on database with " << sender_db->get_item_count()
                                                                   << " items");

            // Copy over the CryptoContext from SenderDB; set the Evaluator for this local instance.
            // Relinearization keys may not have been included in the query. In that case
            // query.relin_keys() simply holds an empty seal::RelinKeys instance. There is no
            // problem with the below call to CryptoContext::set_evaluator.
            CryptoContext crypto_context(sender_db->get_crypto_context());
            crypto_context.set_evaluator(query.relin_keys());

            APSI_LOG_INFO("Crypto context slot count when processing query:");
            APSI_LOG_INFO(crypto_context.encoder()->slot_count());

            // Get the PSIParams
            PSIParams params(sender_db->get_params());

            uint32_t bundle_idx_count = params.bundle_idx_count();
            uint32_t max_items_per_bin = params.table_params().max_items_per_bin;
            uint32_t item_bit_count = params.item_params().item_bit_count;

            // The query response only tells how many ResultPackages to expect; send this first
            uint32_t package_count = safe_cast<uint32_t>(sender_db->get_bin_bundle_count());
            QueryResponse response_query = make_unique<QueryResponse::element_type>();
            response_query->package_count = package_count;

            try {
                send_fun(chl, move(response_query));
            } catch (const exception &ex) {
                APSI_LOG_ERROR(
                    "Failed to send response to query request; function threw an exception: "
                    << ex.what());
                throw;
            }

            vector<CiphertextBits> all_bits(bundle_idx_count);

            // Initialize powers
            for (CiphertextBits &bits : all_bits) {
                // The + 1 is because we index by power. The 0th power is a dummy value. I promise
                // this makes things easier to read.
                size_t bits_size = static_cast<size_t>(item_bit_count);
                bits.reserve(bits_size);
                for (size_t i = 0; i < bits_size; i++) {
                    bits.emplace_back(pool);
                }
            }

            // Load inputs provided in the query
            for (auto &q : query.data()) {
                // The exponent of all the query powers we're about to iterate through
                size_t bit = static_cast<size_t>(q.first);

                // Load bits for all bundle indices i, where e is the exponent specified above
                for (size_t bundle_idx = 0; bundle_idx < all_bits.size(); bundle_idx++) {
                    // Load input^power to all_powers[bundle_idx][exponent]
                    APSI_LOG_DEBUG(
                        "Extracting query ciphertext bit " << bit << " for bundle index "
                                                             << bundle_idx);
                    all_bits[bundle_idx][bit] = move(q.second[bundle_idx]);
                }
            }

            APSI_LOG_INFO("Start processing bin bundles");

            vector<future<void>> futures;
            for (size_t bundle_idx = 0; bundle_idx < bundle_idx_count; bundle_idx++) {
                futures.push_back(tpm.thread_pool().enqueue([&, bundle_idx]() {
                    ProcessBinBundle(
                        sender_db,
                        crypto_context,
                        sender_db->get_bin_bundle(bundle_idx),
                        all_bits,
                        chl,
                        send_rp_fun,
                        static_cast<uint32_t>(bundle_idx),
                        query.compr_mode(),
                        pool);
                }));
            }

            // Wait until all bin bundle caches have been processed
            for (auto &f : futures) {
                f.get();
            }

            APSI_LOG_INFO("Finished processing query request");
        }

        void Sender::ProcessBinBundle(
            const shared_ptr<SenderDB> &sender_db,
            const CryptoContext &crypto_context,
            reference_wrapper<const BinBundle> bundle,
            vector<CiphertextBits> &all_bits,
            Channel &chl,
            function<void(Channel &, ResultPart)> send_rp_fun,
            uint32_t bundle_idx,
            compr_mode_type compr_mode,
            MemoryPoolHandle &pool)
        {
            STOPWATCH(sender_stopwatch, "Sender::ProcessBinBundle");

            // Package for the result data
            auto rp = make_unique<ResultPackage>();
            rp->compr_mode = compr_mode;

            rp->bundle_idx = bundle_idx;

            const BranchingProgram &branching_prog = bundle.get().bp();

            APSI_LOG_INFO("Evaluating branching program with " << all_bits[bundle_idx].size() << " bits");

            rp->psi_result = branching_prog.eval(all_bits[bundle_idx], crypto_context, pool);

            APSI_LOG_INFO("Done evaluating branching program in bundle index " << bundle_idx);

            // Send this result part
            try {
                send_rp_fun(chl, move(rp));
            } catch (const exception &ex) {
                APSI_LOG_ERROR(
                    "Failed to send result part; function threw an exception: " << ex.what());
                throw;
            }
        }
    }
}