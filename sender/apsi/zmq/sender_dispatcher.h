// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

// STD
#include <atomic>
#include <memory>
#include <utility>

// APSI
#include "apsi/network/sender_operation.h"
#include "apsi/network/zmq/zmq_channel.h"
#include "apsi/oprf/oprf_sender.h"
#include "apsi/sender.h"
#include "apsi/sender_db.h"

namespace apsi {
    namespace sender {
        /**
        The ZMQSenderDispatcher is in charge of handling incoming requests through the network.
        */
        class ZMQSenderDispatcher {
        public:
            ZMQSenderDispatcher() = delete;

            /**
            Creates a new ZMQSenderDispatcher object. This constructor accepts a SenderDB object. It
            attempts to retrieve an OPRF key from the SenderDB and uses it to serve OPRF requests.
            This constructor cannot be used if the SenderDB is stripped, because the OPRF key is no
            longer available through the SenderDB.
            */
            ZMQSenderDispatcher(std::shared_ptr<SenderDB> sender_db);

            /**
            Run the dispatcher on the given port.
            */
            void run(const std::atomic<bool> &stop, int port);

        private:
            std::shared_ptr<sender::SenderDB> sender_db_;

            /**
            Dispatch a Get Parameters request to the Sender.
            */
            void dispatch_parms(
                std::unique_ptr<network::ZMQSenderOperation> sop,
                network::ZMQSenderChannel &channel);

            /**
            Dispatch a Query request to the Sender.
            */
            void dispatch_query(
                std::unique_ptr<network::ZMQSenderOperation> sop,
                network::ZMQSenderChannel &channel);
        }; // class ZMQSenderDispatcher
    }      // namespace sender
} // namespace apsi