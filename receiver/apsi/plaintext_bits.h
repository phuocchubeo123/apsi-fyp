#pragma once

// STD
#include <cstdint>
#include <unordered_map>
#include <vector>

// APSI
#include "apsi/crypto_context.h"
#include "apsi/psi_params.h"
#include "apsi/seal_object.h"
#include "apsi/item.h"

// SEAL
#include "seal/ciphertext.h"
#include "seal/modulus.h"

// GSL
#include "gsl/span"

namespace apsi {
    namespace receiver {
        class PlaintextBits {
            public:
            PlaintextBits(
                std::vector<Item> values, const PSIParams &params);

            std::unordered_map<std::uint32_t, SEALObject<seal::Ciphertext>> encrypt(
                const CryptoContext &crypto_context);

            std::string bit_list() const;

            private:
            uint32_t item_bit_count_;
            seal::Modulus mod_;

            std::unordered_map<std::uint32_t, std::vector<std::uint64_t>> bits_;

            void compute_bits(std::vector<Item> values);
        };
    }
}