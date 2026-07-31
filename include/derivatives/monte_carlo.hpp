#pragma once

#include <cstdint>

#include "derivatives/option.hpp"
#include "derivatives/pricing_result.hpp"

namespace derivatives {

enum class VarianceReduction : std::uint8_t {
    None,
    Antithetic,
    ControlVariate,
};

enum class ExecutionPolicy : std::uint8_t {
    Serial,
    OpenMP,
};

/// Simulation budget, reproducibility settings, and execution policy.
struct MonteCarloConfig {
    std::uint64_t paths{};
    std::uint64_t seed{};
    VarianceReduction variance_reduction{VarianceReduction::None};
    ExecutionPolicy execution{ExecutionPolicy::Serial};
    std::uint32_t threads{};
};

/// Estimates a discounted European payoff and its sample uncertainty.
MonteCarloResult price_european_monte_carlo(
    const EuropeanOption& option,
    const MonteCarloConfig& config
);

/// Estimates a discounted arithmetic-average Asian payoff.
MonteCarloResult price_asian_monte_carlo(
    const AsianOption& option,
    const MonteCarloConfig& config
);

/// Estimates a discretely monitored down-and-out payoff with zero rebate.
MonteCarloResult price_down_and_out_monte_carlo(
    const DownAndOutOption& option,
    const MonteCarloConfig& config
);

/// Reports whether this build linked an OpenMP runtime.
[[nodiscard]] bool openmp_available() noexcept;

}  // namespace derivatives
