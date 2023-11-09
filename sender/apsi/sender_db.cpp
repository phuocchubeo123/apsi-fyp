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
                const vector<LocFunc> &hash_funcs, const HashedItem &item)
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
            label_byte_count_ = source.label_byte_count_;
            nonce_byte_count_ = source.nonce_byte_count_;
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
            if (is_labeled()) {
                APSI_LOG_ERROR("Attempted to insert unlabeled data but this is a labeled SenderDB");
                throw logic_error("failed to insert data");
            }

            STOPWATCH(sender_stopwatch, "SenderDB::insert_or_assign (unlabeled)");
            APSI_LOG_INFO("Start inserting " << data.size() << " items in SenderDB");
        }
    }
}