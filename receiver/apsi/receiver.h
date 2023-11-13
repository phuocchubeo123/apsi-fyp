#pragma once

// STD
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// APSI
#include "apsi/crypto_context.h"
#include "apsi/item.h"
#include "apsi/itt.h"
#include "apsi/match_record.h"
#include "apsi/network/channel.h"
#include "apsi/network/network_channel.h"
#include "apsi/oprf/oprf_receiver.h"
#include "apsi/psi_params.h"
#include "apsi/requests.h"
#include "apsi/responses.h"
#include "apsi/seal_object.h"

namespace apsi {
    namespace receiver {
        /**
        The Receiver class implements all necessary functions to create and send parameter, OPRF,
        and PSI or labeled PSI queries (depending on the sender), and process any responses
        received. Most of the member functions are static, but a few (related to creating and
        processing the query itself) require an instance of the class to be created.

        The class includes two versions of an API to performs the necessary operations. The "simple"
        API consists of three functions: Receiver::RequestParams, Receiver::RequestOPRF, and
        Receiver::request_query. However, these functions only support network::NetworkChannel, such
        as network::ZMQChannel, for the communication. Other channels, such as
        network::StreamChannel, are only supported by the "advanced" API.

        */
        class Receiver {
        public:
            /**
            Indicates the number of random-walk steps used by the Kuku library to insert items into
            the cuckoo hash table. Increasing this number can yield better packing rates in cuckoo
            hashing.
            */
            static constexpr std::uint64_t cuckoo_table_insert_attempts = 500;

            /**
            Creates a new receiver with parameters specified. In this case the receiver has
            specified the parameters and expects the sender to use the same set.
            */
            Receiver(PSIParams params);

            /**
            Generates a new set of keys to use for queries.
            */
            void reset_keys();

            /**
            Returns a reference to the CryptoContext for this Receiver.
            */
            const CryptoContext &get_crypto_context() const
            {
                return crypto_context_;
            }

            /**
            Returns a reference to the SEALContext for this Receiver.
            */
            std::shared_ptr<seal::SEALContext> get_seal_context() const
            {
                return crypto_context_.seal_context();
            }

            /**
            Performs a parameter request and returns the received PSIParams object.
            */
            static PSIParams RequestParams(network::NetworkChannel &chl);

            /**
            Performs a PSI or labeled PSI (depending on the sender) query. The query is a vector of
            items, and the result is a same-size vector of MatchRecord objects. If an item is in the
            intersection, the corresponding MatchRecord indicates it in the `found` field, and the
            `label` field may contain the corresponding label if a sender's data included it.
            */
            std::vector<MatchRecord> request_query(
                const std::vector<Item> &items,
                network::NetworkChannel &chl);

            static Request CreateParamsRequest();

            /**
            Creates a Query object from a vector of OPRF hashed items. The query contains the query
            request that can be extracted with the Query::extract_request function and sent to the
            sender with Receiver::SendRequest. It also contains an index translation table that
            keeps track of the order of the hashed items vector, and is used internally by the
            Receiver::process_result_part function to sort the results in the correct order.
            */
            std::pair<Request, IndexTranslationTable> create_query(
                const std::vector<Item> &items);

            /**
            Processes a ResultPart object and returns a vector of MatchRecords in the same order as
            the original vector of OPRF hashed items used to create the query. The return value
            includes matches only for those items whose results happened to be in this particular
            result part. Thus, to determine whether there was a match with the sender's data, the
            results for each received ResultPart must be checked.
            */
            // std::vector<MatchRecord> process_result_part(
            //     const std::vector<LabelKey> &label_keys,
            //     const IndexTranslationTable &itt,
            //     const ResultPart &result_part) const;

            /**
            This function does multiple calls to Receiver::process_result_part, once for each
            ResultPart in the given vector. The results are collected together so that the returned
            vector of MatchRecords reflects the logical OR of the results from each ResultPart.
            */
            // std::vector<MatchRecord> process_result(
            //     const std::vector<LabelKey> &label_keys,
            //     const IndexTranslationTable &itt,
            //     const std::vector<ResultPart> &result) const;

        private:
            void process_result_worker(
                std::atomic<std::uint32_t> &package_count,
                std::vector<MatchRecord> &mrs,
                const std::vector<LabelKey> &label_keys,
                const IndexTranslationTable &itt,
                network::Channel &chl) const;

            void initialize();

            PSIParams params_;

            CryptoContext crypto_context_;

            SEALObject<seal::RelinKeys> relin_keys_;
        }; // class Receiver
    }      // namespace receiver
} // namespace apsi