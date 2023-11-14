#pragma once

#include <vector>
#include <queue>

// APSI
#include "apsi/item.h"
#include "apsi/crypto_context.h"

namespace apsi{
    struct BP{  
    public:
        /*
            Initialize binary tree.
            This tree is implemented in an array-like structure. 
            To initialize it, we create the first element of multiple arrays mentioned in the construction. This first element represents the root node of the tree.
        */
        BP();

        BP(const CryptoContext &crypto_context);

        void clear();

        int addChildNode(int parentNode);
        int32_t addItem(const Item& item);

        seal::Ciphertext eval(
                const std::vector<seal::Ciphertext> &ciphertext_bits,
                seal::MemoryPoolHandle &pool) const;
    
    private:
        int nodes_count_;
        int leaves_count_;

        std::vector<int> id;
        std::vector<int> left_child_;
        std::vector<int> right_child_;
        std::vector<int> parent_;
        std::vector<int> level_;
        std::vector<int> is_leaf_;

        CryptoContext crypto_context_;
    }; 
}