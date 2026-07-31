#include "derivatives/greeks.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace derivatives {

namespace {

double standard_normal_cdf(double value)
{
    return 0.5 * std::erfc(-value / std::sqrt(2.0));
}

double standard_normal_pdf(double value)
{
    return std::exp(-0.5 * value * value)
        / std::sqrt(2.0 * std::numbers::pi_v<double>);
}

}  // namespace

Greeks black_scholes_greeks(const EuropeanOption& option)
{
    validate(option);
    if (option.time_to_maturity == 0.0 || option.volatility == 0.0) {
        throw std::domain_error(
            "Black-Scholes Greeks require positive maturity and volatility"
        );
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
    const double density = standard_normal_pdf(d1);
    const double call_delta = standard_normal_cdf(d1);

    return {
        .delta = option.type == OptionType::Call
            ? call_delta
            : call_delta - 1.0,
        .gamma = density
            / (option.spot * option.volatility * square_root_maturity),
        .vega = option.spot * density * square_root_maturity,
    };
}

}  // namespace derivatives
