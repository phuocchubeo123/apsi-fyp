// STD
#include <algorithm>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <utility>

// APSI
#include "apsi/log.h"
#include "apsi/plaintext_bits.h"
#include "apsi/util/utils.h"

// SEAL
#include "seal/util/uintarithsmallmod.h"

using namespace std;
using namespace seal;
using namespace seal::util;

namespace apsi {
    namespace receiver {
        PlaintextBits::PlaintextBits(
            vector<Item> values, const PSIParams &params)
            : mod_(params.seal_params().plain_modulus()),
            item_bit_count_(params.item_params().item_bit_count)
        {
            compute_bits(move(values));
        }

        unordered_map<uint32_t, SEALObject<Ciphertext>> PlaintextBits::encrypt(
            const CryptoContext &crypto_context)
        {
            if (!crypto_context.encryptor()) {
                throw invalid_argument("encryptor is not set in crypto_context");
            }

            unordered_map<uint32_t, SEALObject<Ciphertext>> result;
            for (auto &p: bits_) {
                Plaintext pt;
                crypto_context.encoder()->encode(p.second, pt);
                result.emplace(
                    make_pair(p.first, crypto_context.encryptor()->encrypt_symmetric(pt)));
            }

            return result;
        }

        void PlaintextBits::compute_bits(vector<Item> values)
        {
            for (int bit = 0; bit < item_bit_count_; bit++){
                vector<bool> bits_vec;
                for (int item_idx = 0; item_idx < values.size(); item_idx++){
                    bits_vec.push_back(values[item_idx].get_bit(bit));
                }
                transform(bits_.begin(), bits_.end(), back_inserter(bits_vec), [](auto &p) {
                    return p.first;
                });
            }
        }
    }
}