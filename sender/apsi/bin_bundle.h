#pragma once

// STD
#include <cstddef>
#include <memory>
#include <vector>

// APSI
#include "apsi/crypto_context.h"
#include "apsi/util/cuckoo_filter.h"
#include "apsi/bp/branching_program.h"

// SEAL
#include "seal/util/uintarithsmallmod.h"
#include "seal/util/uintcore.h"

// GSL
#include "gsl/span"

using namespace apsi::util;

namespace apsi {
    namespace sender {
        struct BinBundleCache{
        };

        /**
        Represents a specific bin bundle and stores the associated data. The type parameter L
        represents the label type. This is either a field element (in the case of labeled PSI), or
        an element of the unit type (in the case of unlabeled PSI).
        */
        class BinBundle {
        public:
            BinBundle();

            BinBundle(const CryptoContext &crypto_context);

            BinBundle(const BinBundle &copy) = delete;

            BinBundle(BinBundle &&source) = default;

            BinBundle &operator=(const BinBundle &assign) = delete;

            BinBundle &operator=(BinBundle &&assign) = default;

            /**
            Inserts item-label pairs into sequential bins, beginning at start_bin_idx. If dry_run is
            specified, no change is made to the BinBundle. On success, returns the size of the
            largest bin bins in the modified range, after insertion has taken place. On failed
            insertion, returns -1. On failure, no modification is made to the BinBundle.
            */
            template <typename T>
            std::int32_t multi_insert(const T &item);

            /**
            Clears the content of the BinBundle
            */
            void clear();

            /**
            Get the number of elements inserted in the BP
            */
            int32_t get_bundle_size(){
                return bundle_size_;
            }

        private:
            /**
            We need this to make Plaintexts
            */
            CryptoContext crypto_context_;

            apsi::BP bp_;

            int32_t bundle_size_;
        };
    }
}