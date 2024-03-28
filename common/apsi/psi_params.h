// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

// STD
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <utility>

// APSI
#include "apsi/log.h"
#include "apsi/util/utils.h"

// Kuku
#include "kuku/kuku.h"

// SEAL
#include "seal/encryptionparams.h"
#include "seal/serialization.h"
#include "seal/util/common.h"

namespace apsi{
    /**
    Contains a collection of parameters required to configure the protocol.
    */
    class PSIParams{
    public:
        /**
        Specifies the Microsoft SEAL encryption parameters for the BFV homomorphic encryption
        scheme.
        */
        class SEALParams : public seal::EncryptionParameters {
        public:
            SEALParams() : seal::EncryptionParameters(seal::scheme_type::bfv)
            {}
        };

        /**
        Item parameters
        */

       struct ItemParams {
            constexpr static std::uint32_t item_bit_count_min = 16;

            constexpr static std::uint32_t item_bit_count_max = 64;

            std::uint32_t item_bit_count;
            std::uint32_t hashed_item_bit_count;
        };

        /**
        Table parameters.
        */
        struct TableParams {
            constexpr static std::uint32_t hash_func_count_min = 1;

            constexpr static std::uint32_t hash_func_count_max = 8;

            /**
            Specified the size of the cuckoo hash table for storing the receiver's items.
            */
            std::uint32_t table_size;

            /**
            The number of hash functions used in receiver's cuckoo hashing.
            */
            std::uint32_t hash_func_count;

            /**
            Suppose some bins are batched into a bundle, there are 2 directions:
            1. Batching receiver's items for ease of evaluation
            2. Batching sender's item to reduce the number of nodes in a BP
            */
            std::uint32_t receiver_bins_per_bundle;
            std::uint32_t sender_bins_per_bundle;

        };

        const TableParams &table_params() const
        {
            return table_params_;
        }

        const ItemParams &item_params() const
        {
            return item_params_;
        }

        const SEALParams &seal_params() const
        {
            return seal_params_;
        }

        std::uint32_t sender_bins_per_bundle() const
        {
            return sender_bins_per_bundle_;
        }

        std::uint32_t receiver_bins_per_bundle() const
        {
            return receiver_bins_per_bundle_;
        }

        std::uint32_t bundle_idx_count() const
        {
            return bundle_idx_count_;
        }

        std::uint32_t item_bit_count() const
        {
            return item_bit_count_;
        }

        PSIParams(
            const TableParams &table_params,
            const ItemParams &item_params,
            const SEALParams &seal_params)
            : table_params_(table_params), 
              item_params_(item_params),
              seal_params_(seal_params)
        {
            APSI_LOG_INFO("Begin initializing PSIParams from loaded parameters...")
            initialize();
        }

        PSIParams(const PSIParams &copy) = default;

        PSIParams &operator=(const PSIParams &copy) = default;

        std::string to_string() const;

        /**
        Writes the PSIParams to a stream.
        */
        std::size_t save(std::ostream &out) const;

        /**
        Reads the PSIParams from a stream.
        */
        static std::pair<PSIParams, std::size_t> Load(std::istream &in);

        /**
        Reads the PSIParams from a JSON string
        */
        static PSIParams Load(const std::string &in);

    private:
        TableParams table_params_;

        ItemParams item_params_;

        SEALParams seal_params_;

        std::uint32_t sender_bins_per_bundle_;

        std::uint32_t receiver_bins_per_bundle_;

        std::uint32_t bundle_idx_count_;

        std::uint32_t item_bit_count_;

        std::uint32_t max_bins_per_bundle_;

        void initialize();
    };
}