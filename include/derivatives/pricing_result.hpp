#pragma once

#include <cstdint>

namespace derivatives {

/// Estimate, uncertainty, and reproducibility metadata from Monte Carlo pricing.
struct MonteCarloResult {
    double price{};
    double standard_error{};
    double confidence_interval_lower{};
    double confidence_interval_upper{};
    std::uint64_t paths{};
    std::uint64_t seed{};
    std::uint64_t effective_samples{};
    std::uint32_t threads_used{1U};
    double control_variate_beta{};
};

}  // namespace derivatives
