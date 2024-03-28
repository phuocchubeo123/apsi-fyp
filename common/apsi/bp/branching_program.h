#pragma once

#include <vector>
#include <queue>
#include <cmath>

// APSI
#include "apsi/item.h"
#include "apsi/crypto_context.h"
#include "apsi/log.h"
#include "apsi/plaintext_bits.h"

namespace apsi{
    struct BranchingProgram{  
    public:
        /*
            Initialize binary tree.
            This tree is implemented in an array-like structure. 
            To initialize it, we create the first element of multiple arrays mentioned in the construction. This first element represents the root node of the tree.
        */
        BranchingProgram();

        BranchingProgram(const CryptoContext &crypto_context);

        void clear();

        int addChildNode(int parentNode);

        template <typename T>
        int32_t addItem(const T &item);

        seal::Ciphertext eval(
            const std::vector<seal::Ciphertext> &ciphertext_bits,
            const CryptoContext &crypto_context,
            seal::MemoryPoolHandle &pool) const;

        seal::Ciphertext eval_less_mult(
            const std::vector<seal::Ciphertext> &ciphertext_bits,
            const CryptoContext &crypto_context,
            seal::MemoryPoolHandle &pool) const;

        void patterson_stockmeyer(uint32_t d);
            
    private:
        int nodes_count_;
        int leaves_count_;

        std::vector<int> id;

        CryptoContext crypto_context_;

        seal::Plaintext tot_ptx;

        seal::Plaintext zero_ptx;

        seal::Plaintext one_ptx;

        std::vector<PlaintextBits> item_;
    }; 
}