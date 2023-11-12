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
    } // namespace

    namespace receiver {
        size_t IndexTranslationTable::find_item_idx(size_t table_idx) const noexcept
        {
            auto item_idx = table_idx_to_item_idx_.find(table_idx);
            if (item_idx == table_idx_to_item_idx_.cend()) {
                return item_count();
            }

            return item_idx->second;
        }

        // Receiver::Receiver(PSIParams params) : params_(move(params))
        // {
        //     initialize();
        // }

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

            // Wait for a valid message of the right type
            ParamsResponse response;
            bool logged_waiting = false;

            while (!(response = to_params_response(chl.receive_response()))) {
                if (!logged_waiting) {
                    // We want to log 'Waiting' only once, even if we have to wait for several
                    // sleeps.
                    logged_waiting = true;
                    APSI_LOG_INFO("Waiting for response to parameter request");
                }

                this_thread::sleep_for(50ms);
            }


            return *response->params;
        }
    }
}