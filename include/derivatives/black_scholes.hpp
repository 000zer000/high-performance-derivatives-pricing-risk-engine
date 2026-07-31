#pragma once

#include "derivatives/option.hpp"

namespace derivatives {

/// Prices a validated no-dividend European call or put analytically.
double black_scholes_price(const EuropeanOption& option);

}  // namespace derivatives
