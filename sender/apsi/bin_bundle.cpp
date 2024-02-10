// STD
#include <algorithm>
#include <functional>
#include <future>
#include <type_traits>
#include <utility>

// APSI
#include "apsi/bin_bundle.h"
#include "apsi/bin_bundle_generated.h"
#include "apsi/thread_pool_mgr.h"
#include "apsi/util/interpolate.h"
#include "apsi/util/utils.h"

// SEAL
#include "seal/util/defines.h"

namespace apsi {
    using namespace std;
    using namespace seal;
    using namespace seal::util;
    using namespace util;
    namespace sender{
        BinBundle::BinBundle(): 
            bp_(), crypto_context_()
        {
            // Set up internal data structures
            clear();
        }

        BinBundle::BinBundle(const CryptoContext &crypto_context): 
            crypto_context_(crypto_context), 
            bp_(crypto_context)
        {
            // Set up internal data structures
            clear();
        }

        void BinBundle::clear(){
        }

        template <>
        int32_t BinBundle::multi_insert(
            const PlaintextBits &item
        )
        {
            int32_t new_bp_size = bp_.addItem(item);
            bundle_size_ = new_bp_size;
            return new_bp_size;
        }
    }
}