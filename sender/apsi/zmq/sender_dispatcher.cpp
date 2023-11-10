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
        }
    }
}