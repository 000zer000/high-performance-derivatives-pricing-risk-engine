#pragma once

#include "derivatives/option.hpp"

namespace derivatives {

double black_scholes_price(const EuropeanOption& option);

}  // namespace derivatives
