// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

// STD
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// Kuku
#include "kuku/common.h"

// GSL
#include "gsl/span"

namespace apsi {
    class Item {
    public:
        using value_type = std::vector<bool>;
        using value_hashed_type = std::array<unsigned char, 16>;

        /**
        Constructs a zero item.
        */
        Item() = default;

        Item(const value_type &value) : value_(value)
        {}

        Item(const Item &) = default;

        Item(Item &&) = default;

        Item &operator=(const Item &item) = default;

        Item &operator=(Item &&item) = default;

        /**
        Constructs an Item by hashing a given string of arbitrary length.
        */
        Item(const std::basic_string<char> &str)
        {
            operator=(str);
        }

        /**
        Hash a given string of arbitrary length into an Item.
        */
        Item &operator=(const std::basic_string<char> &str);

        bool operator==(const Item &other) const
        {
            return value_ == other.value_;
        }

        value_type value() const
        {
            return value_;
        }

        value_type &value()
        {
            return value_;
        }

        bool get_bit(short bit) const{
            return value_[bit];
        }

        int get_length() const{
            return value_.size();
        }

        std::string raw_val() const{
            return raw_val_;
        }

        /**
        Returns a span of a desired (standard layout) type to the label data.
        */
        template <typename T, typename = std::enable_if_t<std::is_standard_layout<T>::value>>
        auto get_as() const
        {
            constexpr std::size_t count = sizeof(value_hashed_) / sizeof(T);
            return gsl::span<std::add_const_t<T>, count>(
                reinterpret_cast<std::add_const_t<T> *>(value_hashed_.data()), count);
        }

        /**
        Returns a span of a desired (standard layout) type to the label data.
        */
        template <typename T, typename = std::enable_if_t<std::is_standard_layout<T>::value>>
        auto get_as()
        {
            constexpr std::size_t count = sizeof(value_hashed_) / sizeof(T);
            return gsl::span<T, count>(reinterpret_cast<T *>(value_hashed_.data()), count);
        }

        std::string to_string() const;

        uint32_t int_val;

    private:
        void hash_to_value(const void *in, std::size_t size);
        value_type value_{};
        value_hashed_type value_hashed_{};
        std::string raw_val_;

    }; // class Item

    /**
    Represents an Item that has been hashed with an OPRF.
    */
    class HashedItem : public Item {
    public:
        using Item::Item;
    };

    /**
    Represents an arbitrary-length label.
    */
    using Label = std::vector<unsigned char>;

    /**
    Represents an encrypted Label.
    */
    class EncryptedLabel : public Label {
    public:
        using Label::Label;
    };

    /**
    The byte count of label encryption keys.
    */
    constexpr std::size_t label_key_byte_count = 16;

    /**
    The maximal/default byte count of label encryption nonces.
    */
    constexpr std::size_t max_nonce_byte_count = 16;

    /**
    Represents a label encryption key.
    */
    using LabelKey = std::array<unsigned char, label_key_byte_count>;
} // namespace apsi