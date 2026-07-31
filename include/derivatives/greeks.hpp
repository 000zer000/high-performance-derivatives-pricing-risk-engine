#pragma once

#include "derivatives/option.hpp"

namespace derivatives {

/// Black-Scholes sensitivities; vega is per unit change in volatility.
struct Greeks {
    double delta{};
    double gamma{};
    double vega{};
};

/// Computes delta, gamma, and vega for positive maturity and volatility.
Greeks black_scholes_greeks(const EuropeanOption& option);

}  // namespace derivatives
