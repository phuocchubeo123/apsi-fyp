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
        private:
            /**
            This is true iff cache_ needs to be regenerated
            */
            bool cache_invalid_;

            /**
            We need this to make Plaintexts
            */
            CryptoContext crypto_context_;

            apsi::BP bp_;
        };
    }
}