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
    /**
    Identical to Bitstring, except the underlying data is not owned.
    */
    template <
        typename T,
        typename = std::enable_if_t<
            std::is_same<T, unsigned char>::value || std::is_same<T, const unsigned char>::value>>
    class BitstringView {
    private:
        gsl::span<T> data_;

        std::uint32_t bit_count_;

    public:
        BitstringView(gsl::span<T> data, std::uint32_t bit_count)
        {
            // Sanity check: bit_count cannot be 0
            if (!bit_count) {
                throw std::invalid_argument("bit_count must be positive");
            }
            // Sanity check: bit_count cannot exceed underlying data length
            if (data.size() * 8 < bit_count) {
                throw std::invalid_argument("bit_count exceeds the data length");
            }

            // Now move
            size_t data_length = (bit_count + 7) / 8;
            if (data.size() == data_length) {
                data_ = std::move(data);
            } else {
                data_ = data.subspan(0, data_length);
            }

            bit_count_ = bit_count;
        }

        BitstringView(T *data, std::uint32_t bit_count)
            : BitstringView(gsl::span<T>{ data, (bit_count + 7) / 8 }, bit_count)
        {}

        template <typename S>
        BitstringView(const BitstringView<S> &view)
        {
            data_ = static_cast<gsl::span<S>>(view.data());
            bit_count_ = view.bit_count();
        }

        bool operator==(const BitstringView<T> &rhs) const
        {
            // Check equivalence of pointers
            return (bit_count_ == rhs.bit_count_) && (data_.data() == rhs.data_.data());
        }

        std::uint32_t bit_count() const
        {
            return bit_count_;
        }

        /**
        Returns a reference to the underlying bytes.
        */
        gsl::span<T> data() const
        {
            return { data_.data(), data_.size() };
        }
    };

    /**
    Represents a bitstring, i.e., a string of bytes that tells you how many bits it's supposed to be
    interpreted as. The stated bit_count must be at most the number of actual underlying bits.
    */
    class Bitstring {
    private:
        std::vector<unsigned char> data_;

        std::uint32_t bit_count_;

    public:
        Bitstring(std::vector<unsigned char> &&data, std::uint32_t bit_count)
        {
            // Sanity check: bit_count cannot be 0
            if (!bit_count) {
                throw std::invalid_argument("bit_count must be positive");
            }

            // Sanity check: bit_count cannot exceed underlying data length
            if (data.size() * 8 < bit_count) {
                throw std::invalid_argument("bit_count exceeds the data length");
            }

            // Now move
            data_ = std::move(data);
            size_t data_length = (bit_count + 7) / 8;
            if (data_length < data_.size()) {
                data_.resize(data_length);
            }

            bit_count_ = bit_count;
        }

        bool operator==(const Bitstring &rhs) const
        {
            return (bit_count_ == rhs.bit_count_) && (data_ == rhs.data_);
        }

        std::uint32_t bit_count() const
        {
            return bit_count_;
        }

        /**
        Returns a BitstringView representing the same underlying data.
        */
        BitstringView<unsigned char> to_view()
        {
            return { data(), bit_count_ };
        }

        /**
        Returns a BitstringView representing the same underlying data.
        */
        BitstringView<const unsigned char> to_view() const
        {
            return { data(), bit_count_ };
        }

        /**
        Returns a reference to the underlying bytes.
        */
        gsl::span<unsigned char> data()
        {
            return { data_.data(), data_.size() };
        }

        /**
        Returns a reference to the underlying bytes.
        */
        gsl::span<const unsigned char> data() const
        {
            return { data_.data(), data_.size() };
        }

        /**
        Releases ownership of, and returns the data as a std::vector<unsigned char>.
        */
        std::vector<unsigned char> release()
        {
            bit_count_ = 0;
            return std::move(data_);
        }
    };

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

        /**
        Returns the Bitstring representing this Item's data.
        */
        Bitstring to_bitstring(std::uint32_t item_bit_count) const;

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

    private:
        void hash_to_value(const void *in, std::size_t size);
        value_type value_{};
        value_hashed_type value_hashed_{};

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