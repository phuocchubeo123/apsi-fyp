#pragma once

// STD
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace apsi{
    namespace util{
        /**
        Convert the given input to digits.
        */
        std::vector<std::uint64_t> conversion_to_digits(
            const std::uint64_t input, const std::uint64_t base);
    }
}