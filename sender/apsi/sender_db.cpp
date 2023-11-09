// STD
#include <algorithm>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>

// APSI
#include "apsi/psi_params.h"
#include "apsi/sender_db.h"
#include "apsi/sender_db_generated.h"
#include "apsi/thread_pool_mgr.h"
#include "apsi/util/label_encryptor.h"
#include "apsi/util/utils.h"

// Kuku
#include "kuku/locfunc.h"

// SEAL
#include "seal/util/common.h"
#include "seal/util/streambuf.h"

using namespace std;
using namespace seal;
using namespace seal::util;
using namespace kuku;

namespace apsi {
    using namespace util;
    using namespace oprf;

    namespace sender {
        namespace {
            /**
            Creates and returns the vector of hash functions similarly to how Kuku 2.x sets them
            internally.
            */
            vector<LocFunc> hash_functions(const PSIParams &params)
            {
                vector<LocFunc> result;
                for (uint32_t i = 0; i < params.table_params().hash_func_count; i++) {
                    result.emplace_back(params.table_params().table_size, make_item(i, 0));
                }

                return result;
            }

            /**
            Computes all cuckoo hash table locations for a given item.
            */
            unordered_set<location_type> all_locations(
                const vector<LocFunc> &hash_funcs, const Item &item)
            {
                unordered_set<location_type> result;
                for (auto &hf : hash_funcs) {
                    result.emplace(hf(item.get_as<kuku::item_type>().front()));
                }

                return result;
            }

            /**
            Unpacks a cuckoo idx into its bin and bundle indices
            */
            pair<size_t, size_t> unpack_cuckoo_idx(size_t cuckoo_idx, size_t bins_per_bundle)
            {
                // Recall that bin indices are relative to the bundle index. That is, the first bin
                // index of a bundle at bundle index 5 is 0. A cuckoo index is similar, except it is
                // not relative to the bundle index. It just keeps counting past bundle boundaries.
                // So in order to get the bin index from the cuckoo index, just compute cuckoo_idx
                // (mod bins_per_bundle).
                size_t bin_idx = cuckoo_idx % bins_per_bundle;

                // Compute which bundle index this cuckoo index belongs to
                size_t bundle_idx = (cuckoo_idx - bin_idx) / bins_per_bundle;

                return { bin_idx, bundle_idx };
            }

            /**
            Compute each Item's cuckoo index
            */
            vector<pair<Item, size_t>> preprocess_unlabeled_data(
                const vector<Item>::const_iterator begin,
                const vector<Item>::const_iterator end,
                const PSIParams &params)
            {
                STOPWATCH(sender_stopwatch, "preprocess_unlabeled_data");
                APSI_LOG_DEBUG(
                    "Start preprocessing " << distance(begin, end) << " unlabeled items");
                
                // Some variables we'll need
                size_t item_bit_count = params.item_bit_count();

                // Set up Kuku hash functions
                auto hash_funcs = hash_functions(params);

                // Calculate the cuckoo indices for each item. Store every pair of (item-label,
                // cuckoo_idx) in a vector. Later, we're gonna sort this vector by cuckoo_idx and
                // use the result to parallelize the work of inserting the items into BinBundles.
                vector<pair<Item, size_t>> data_with_indices;
                for (auto it = begin; it != end; it++) {
                    const Item &item = *it;

                    // Get the cuckoo table locations for this item and add to data_with_indices
                    for (auto location : all_locations(hash_funcs, item)) {
                        // The current hash value is an index into a table of Items. In reality our
                        // BinBundles are tables of bins, which contain chunks of items. How many
                        // chunks? bins_per_item many chunks
                        size_t bin_idx = location;

                        // Store the data along with its index
                        data_with_indices.emplace_back(make_pair(item, bin_idx));
                    }
                }

                return data_with_indices;
            }

            /**
            Inserts the given items and corresponding labels into bin_bundles at their respective
            cuckoo indices. It will only insert the data with bundle index in the half-open range
            range indicated by work_range. If inserting into a BinBundle would make the number of
            items in a bin larger than max_bin_size, this function will create and insert a new
            BinBundle. If overwrite is set, this will overwrite the labels if it finds an
            AlgItemLabel that matches the input perfectly.
            */
            template <typename T>
            void insert_or_assign_worker(
                const vector<pair<T, size_t>> &data_with_indices,
                vector<BinBundle> &bin_bundles,
                CryptoContext &crypto_context,
                uint32_t bundle_index,
                uint32_t receiver_bins_per_bundle,
                size_t max_bin_size,
                bool compressed)
            {
                STOPWATCH(sender_stopwatch, "insert_or_assign_worker");
                APSI_LOG_DEBUG(
                    "Insert-or-Assign worker for bundle index "
                    << bundle_index << "; mode of operation: ");

                // Iteratively insert each item-label pair at the given cuckoo index
                for (auto &data_with_idx : data_with_indices) {
                    const T &data = data_with_idx.first;

                    // Get the bundle index
                    size_t cuckoo_idx = data_with_idx.second;
                    size_t bin_idx, bundle_idx;
                    tie(bin_idx, bundle_idx) = unpack_cuckoo_idx(cuckoo_idx, receiver_bins_per_bundle);

                    // If the bundle_idx isn't in the prescribed range, don't try to insert this
                    // data
                    if (bundle_idx != bundle_index) {
                        // Dealing with this bundle index is not our job
                        continue;
                    }

                    // Get the bundle set at the given bundle index
                    BinBundle &bundle = bin_bundles[bundle_idx];

                }
            }

            /**
            Takes data to be inserted, splits it up, and distributes it so that
            thread_count many threads can all insert in parallel. If overwrite is set, this will
            overwrite the labels if it finds an AlgItemLabel that matches the input perfectly.
            */
            template <typename T>
            void dispatch_insert_or_assign(
                vector<pair<T, size_t>> &data_with_indices,
                vector<BinBundle> &bin_bundles,
                CryptoContext &crypto_context,
                uint32_t receiver_bins_per_bundle,
                uint32_t max_bin_size,
                bool compressed)
            {
                ThreadPoolMgr tpm;
                
                // Collect the bundle indices and partition them into thread_count many partitions.
                // By some uniformity assumption, the number of things to insert per partition
                // should be roughly the same. Note that the contents of bundle_indices is always
                // sorted (increasing order).
                set<size_t> bundle_indices_set;
                for (auto &data_with_idx: data_with_indices){
                    size_t cuckoo_idx = data_with_idx.second;
                    size_t bin_idx, bundle_idx;
                    tie(bin_idx, bundle_idx) = unpack_cuckoo_idx(cuckoo_idx, receiver_bins_per_bundle);
                    bundle_indices_set.insert(bundle_idx);
                }

                vector<size_t> bundle_indices;
                bundle_indices.reserve(bundle_indices_set.size());
                copy(
                    bundle_indices_set.begin(),
                    bundle_indices_set.end(),
                    back_inserter(bundle_indices));
                sort(bundle_indices.begin(), bundle_indices.end());

                // Run the threads on the partitions
                vector<future<void>> futures(bundle_indices.size());
                APSI_LOG_INFO(
                    "Launching " << bundle_indices.size() << " insert-or-assign worker tasks");
                size_t future_idx = 0;

                for (auto &bundle_idx : bundle_indices) {
                    futures[future_idx++] = tpm.thread_pool().enqueue([&, bundle_idx]() {
                        insert_or_assign_worker(
                            data_with_indices,
                            bin_bundles,
                            crypto_context,
                            static_cast<uint32_t>(bundle_idx),
                            receiver_bins_per_bundle,
                            max_bin_size,
                            compressed);
                    });
                }

                // Wait for the tasks to finish
                for (auto &f : futures) {
                    f.get();
                }

                APSI_LOG_INFO("Finished insert-or-assign worker tasks");
            }
        }

        /**
        Creating sender DB in non labeled mode, do not need OPRF
        */
        SenderDB::SenderDB(
            PSIParams params, bool compressed)
            : params_(params), crypto_context_(params_), 
              item_count_(0),
              compressed_(compressed)
        {
            // Set the evaluator. This will be used for BatchedPlaintextPolyn::eval.
            crypto_context_.set_evaluator();

            // Reset the SenderDB data structures
            clear();
        }

        SenderDB &SenderDB::operator=(SenderDB &&source)
        {
            // Do nothing if moving to self
            if (&source == this) {
                return *this;
            }

            // Lock the current SenderDB
            auto this_lock = get_writer_lock();

            params_ = source.params_;
            crypto_context_ = source.crypto_context_;
            item_count_ = source.item_count_;
            compressed_ = source.compressed_;
            stripped_ = source.stripped_;

            // Lock the source before moving stuff over
            auto source_lock = source.get_writer_lock();

            bin_bundles_ = move(source.bin_bundles_);
            oprf_key_ = move(source.oprf_key_);
            source.oprf_key_ = OPRFKey();

            // Reset the source data structures
            source.clear_internal();

            return *this;
        }

        void SenderDB::clear_internal()
        {
            // Assume the SenderDB is already locked for writing

            // Clear the set of inserted items
            item_count_ = 0;

            // Clear the BinBundles
            bin_bundles_.clear();
            bin_bundles_.resize(params_.bundle_idx_count());

            // Reset the stripped_ flag
            stripped_ = false;
        }

        void SenderDB::clear()
        {
            // Lock the database for writing
            auto lock = get_writer_lock();

            clear_internal();
        }

        void SenderDB::insert_or_assign(const vector<Item> &data)
        {
            if (stripped_) {
                APSI_LOG_ERROR("Cannot insert data to a stripped SenderDB");
                throw logic_error("failed to insert data");
            }

            STOPWATCH(sender_stopwatch, "SenderDB::insert_or_assign (unlabeled)");
            APSI_LOG_INFO("Start inserting " << data.size() << " items in SenderDB");

            vector<pair<Item, size_t>> data_with_indices =
                preprocess_unlabeled_data(data.begin(), data.end(), params_);

            uint32_t max_bin_size = params_.table_params().max_items_per_bin;
            uint32_t receiver_bins_per_bundle = params_.table_params().receiver_bins_per_bundle;
            uint32_t sender_bins_per_bundle = params_.table_params().sender_bins_per_bundle;

            dispatch_insert_or_assign(
                data_with_indices,
                bin_bundles_,
                crypto_context_,
                receiver_bins_per_bundle,
                max_bin_size,
                compressed_);
        }
    }
}