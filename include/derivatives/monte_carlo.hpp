#pragma once

#include <cstdint>

#include "derivatives/option.hpp"
#include "derivatives/pricing_result.hpp"

namespace derivatives {

struct MonteCarloConfig {
    std::uint64_t paths{};
    std::uint64_t seed{};
};

MonteCarloResult price_european_monte_carlo(
    const EuropeanOption& option,
    const MonteCarloConfig& config
);

}  // namespace derivatives
