#pragma once

#include <cstdint>

#include "derivatives/option.hpp"
#include "derivatives/pricing_result.hpp"

namespace derivatives {

/// Serial simulation budget and caller-controlled random seed.
struct MonteCarloConfig {
    std::uint64_t paths{};
    std::uint64_t seed{};
};

/// Estimates a discounted European payoff and its sample uncertainty.
MonteCarloResult price_european_monte_carlo(
    const EuropeanOption& option,
    const MonteCarloConfig& config
);

}  // namespace derivatives
