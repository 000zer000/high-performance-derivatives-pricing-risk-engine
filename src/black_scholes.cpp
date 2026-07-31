#include "derivatives/black_scholes.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace derivatives {

namespace {

double standard_normal_cdf(double value)
{
    return 0.5 * std::erfc(-value / std::sqrt(2.0));
}

double intrinsic_value(const EuropeanOption& option)
{
    if (option.type == OptionType::Call) {
        return std::max(option.spot - option.strike, 0.0);
    }
    return std::max(option.strike - option.spot, 0.0);
}

double checked_price(double price)
{
    if (!std::isfinite(price)) {
        throw std::overflow_error("Black-Scholes price is outside the finite double range");
    }
    return std::max(price, 0.0);
}

}  // namespace

double black_scholes_price(const EuropeanOption& option)
{
    validate(option);

    if (option.time_to_maturity == 0.0) {
        return intrinsic_value(option);
    }

    const double discount_factor =
        std::exp(-option.risk_free_rate * option.time_to_maturity);
    const double discounted_strike = option.strike * discount_factor;

    if (option.volatility == 0.0) {
        const double deterministic_price = option.type == OptionType::Call
            ? option.spot - discounted_strike
            : discounted_strike - option.spot;
        return checked_price(deterministic_price);
    }

    const double square_root_maturity = std::sqrt(option.time_to_maturity);
    const double volatility_over_horizon =
        option.volatility * square_root_maturity;
    const double d1 =
        (std::log(option.spot) - std::log(option.strike)
         + (option.risk_free_rate
            + 0.5 * option.volatility * option.volatility)
             * option.time_to_maturity)
        / volatility_over_horizon;
    const double d2 = d1 - volatility_over_horizon;

    if (option.type == OptionType::Call) {
        return checked_price(
            option.spot * standard_normal_cdf(d1)
            - discounted_strike * standard_normal_cdf(d2)
        );
    }
    return checked_price(
        discounted_strike * standard_normal_cdf(-d2)
        - option.spot * standard_normal_cdf(-d1)
    );
}

}  // namespace derivatives
