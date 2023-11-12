// STD
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <thread>

// APSI
#include "apsi/log.h"
#include "apsi/oprf/oprf_sender.h"
#include "apsi/requests.h"
#include "apsi/zmq/sender_dispatcher.h"

// SEAL
#include "seal/util/common.h"

using namespace std;
using namespace seal;
using namespace seal::util;

namespace apsi {
    using namespace network;
    using namespace oprf;

    namespace sender {
        ZMQSenderDispatcher::ZMQSenderDispatcher(shared_ptr<SenderDB> sender_db, OPRFKey oprf_key)
            : sender_db_(move(sender_db)), oprf_key_(move(oprf_key))
        {
            if (!sender_db_) {
                throw invalid_argument("sender_db is not set");
            }
        }

        ZMQSenderDispatcher::ZMQSenderDispatcher(shared_ptr<SenderDB> sender_db)
            : sender_db_(move(sender_db))
        {
            if (!sender_db_) {
                throw invalid_argument("sender_db is not set");
            }
        }

        void ZMQSenderDispatcher::run(const atomic<bool> &stop, int port)
        {
            ZMQSenderChannel chl;
            stringstream ss;
            ss << "tcp://*:" << port;

            APSI_LOG_INFO("ZMQSenderDispatcher listening on port " << port);
            chl.bind(ss.str());

            auto seal_context = sender_db_->get_seal_context();
            APSI_LOG_INFO("Done setting SEAL context, ready for query");

            // Run until stopped
            bool logged_waiting = false;

            while (!stop) {
                unique_ptr<ZMQSenderOperation> sop;
                if (!(sop = chl.receive_network_operation(seal_context))) {
                    if (!logged_waiting) {
                        // We want to log 'Waiting' only once, even if we have to wait
                        // for several sleeps. And only once after processing a request as well.
                        logged_waiting = true;
                        APSI_LOG_INFO("Waiting for request from Receiver");
                    }

                    this_thread::sleep_for(50ms);
                    continue;
                }

                switch (sop->sop->type()) {
                case SenderOperationType::sop_parms:
                    APSI_LOG_INFO("Received parameter request");
                    dispatch_parms(move(sop), chl);
                    break;
                default:
                    // We should never reach this point
                    throw runtime_error("invalid operation");
                }

                logged_waiting = false;
            }
        }

        void ZMQSenderDispatcher::dispatch_parms(
            unique_ptr<ZMQSenderOperation> sop, ZMQSenderChannel &chl)
        {
            STOPWATCH(sender_stopwatch, "ZMQSenderDispatcher::dispatch_params");

            try {
                // Extract the parameter request
                ParamsRequest params_request = to_params_request(move(sop->sop));

                Sender::RunParams(
                    params_request,
                    sender_db_,
                    chl,
                    [&sop](Channel &c, unique_ptr<SenderOperationResponse> sop_response) {
                        auto nsop_response = make_unique<ZMQSenderOperationResponse>();
                        nsop_response->sop_response = move(sop_response);
                        nsop_response->client_id = move(sop->client_id);

                        // We know for sure that the channel is a SenderChannel so use static_cast
                        static_cast<ZMQSenderChannel &>(c).send(move(nsop_response));
                    });
            } catch (const exception &ex) {
                APSI_LOG_ERROR(
                    "Sender threw an exception while processing parameter request: " << ex.what());
            }
        }
    }
}