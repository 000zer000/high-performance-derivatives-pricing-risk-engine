#pragma once

#include <cstdint>

namespace derivatives {

struct MonteCarloResult {
    double price{};
    double standard_error{};
    double confidence_interval_lower{};
    double confidence_interval_upper{};
    std::uint64_t paths{};
    std::uint64_t seed{};
};

}  // namespace derivatives
