// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

// STD
#include <iterator>
#include <sstream>

// APSI
#include "apsi/item.h"
#include "apsi/util/utils.h"

// SEAL
#include "seal/util/blake2.h"

using namespace std;

int charToByte[60] = {203, 64, 16, 96, 77, 150, 63, 27, 186, 126, 50, 132, 29, 206, 184, 213, 251, 173, 41, 22, 246, 137, 247, 44, 47, 176, 0, 0, 0, 0, 0, 0, 162, 112, 160, 100, 79, 106, 194, 125, 254, 238, 207, 196, 38, 241, 228, 182, 90, 2, 212, 244, 103, 104, 216, 89, 39, 28, 0, 0};

namespace apsi {
    Item& Item::operator=(const std::basic_string<char> &str)
    {
        if (str.empty()) {
            throw std::invalid_argument("str cannot be empty");
        }

        hash_to_value(str.data(), str.size() * sizeof(char));

        for (char c: str){
            value_.push_back(c - '0');
        }
        return *this;
    }

    void Item::hash_to_value(const void *in, size_t size)
    {
        APSI_blake2b(value_hashed_.data(), sizeof(value_hashed_), in, size, nullptr, 0);
    }

    Bitstring Item::to_bitstring(uint32_t item_bit_count) const
    {
        vector<unsigned char> bytes;
        bytes.reserve(sizeof(value_type));
        copy(value_.cbegin(), value_.cend(), back_inserter(bytes));
        return { move(bytes), item_bit_count };
    }

    string Item::to_string() const
    {
        string s;
        for (bool b: value_) s.push_back('0' + b);
        return s;
    }
} // namespace apsi