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
            SEALParams() : seal::EncryptionParameters(seal::scheme_type::bgv)
            {}
        };

        constexpr static std::uint32_t item_bit_count_min = 32;

        constexpr static std::uint32_t item_bit_count_max = 128;

        /**
        Parameters describing the item and label properties.
        */
        struct ItemParams {
            constexpr static std::uint32_t felts_per_item_max = 32;

            constexpr static std::uint32_t felts_per_item_min = 1;

            /**
            Specified how many SEAL batching slots are occupied by an item.
            */
            std::uint32_t felts_per_item;
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
            Specifies the number of sender's items stored in a single hash table bin. A larger value
            requires a deeper encrypted computation, or more powers of the encrypted query to be
            sent from the receiver to the sender, but reduces the number of ciphertexts sent from
            the sender to the receiver.
            */
            std::uint32_t max_items_per_bin;

            /**
            The number of hash functions used in receiver's cuckoo hashing.
            */
            std::uint32_t hash_func_count;
        };

        const ItemParams &item_params() const
        {
            return item_params_;
        }

        const TableParams &table_params() const
        {
            return table_params_;
        }

        const SEALParams &seal_params() const
        {
            return seal_params_;
        }

        std::uint32_t items_per_bundle() const
        {
            return items_per_bundle_;
        }

        std::uint32_t bins_per_bundle() const
        {
            return bins_per_bundle_;
        }

        std::uint32_t bundle_idx_count() const
        {
            return bundle_idx_count_;
        }

        std::uint32_t item_bit_count() const
        {
            return item_bit_count_;
        }

        std::uint32_t item_bit_count_per_felt() const
        {
            return item_bit_count_per_felt_;
        }

        PSIParams(
            const ItemParams &item_params,
            const TableParams &table_params,
            const SEALParams &seal_params)
            : item_params_(item_params), table_params_(table_params), 
              seal_params_(seal_params)
        {
            initialize();
        }

        PSIParams(const PSIParams &copy) = default;

        PSIParams &operator=(const PSIParams &copy) = default;

        std::string to_string() const;

        /**
        Returns an approximate base-2 logarithm of the false-positive probability per receiver's
        item.
        */
        double log2_fpp() const
        {
            return std::min<double>(
                0.0,
                (-static_cast<double>(item_bit_count_per_felt_) +
                 std::log2(static_cast<double>(table_params_.max_items_per_bin))) *
                    item_params_.felts_per_item);
        }

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
        ItemParams item_params_;

        TableParams table_params_;

        SEALParams seal_params_;

        std::uint32_t items_per_bundle_;

        std::uint32_t bins_per_bundle_;

        std::uint32_t bundle_idx_count_;

        std::uint32_t item_bit_count_;

        std::uint32_t item_bit_count_per_felt_;

        void initialize();
    };
}