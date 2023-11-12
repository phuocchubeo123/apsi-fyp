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
    }
}