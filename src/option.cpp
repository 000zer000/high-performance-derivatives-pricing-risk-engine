#include "derivatives/option.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>

namespace derivatives {

namespace {

void require_finite(double value, std::string_view field_name)
{
    if (!std::isfinite(value)) {
        throw std::invalid_argument(std::string{field_name} + " must be finite");
    }
}

}  // namespace

void validate(const EuropeanOption& option)
{
    require_finite(option.spot, "spot");
    require_finite(option.strike, "strike");
    require_finite(option.risk_free_rate, "risk_free_rate");
    require_finite(option.volatility, "volatility");
    require_finite(option.time_to_maturity, "time_to_maturity");

    if (option.spot <= 0.0) {
        throw std::invalid_argument("spot must be greater than zero");
    }
    if (option.strike <= 0.0) {
        throw std::invalid_argument("strike must be greater than zero");
    }
    if (option.volatility < 0.0) {
        throw std::invalid_argument("volatility must be non-negative");
    }
    if (option.time_to_maturity < 0.0) {
        throw std::invalid_argument("time_to_maturity must be non-negative");
    }
    if (option.type != OptionType::Call && option.type != OptionType::Put) {
        throw std::invalid_argument("option type must be call or put");
    }
}

}  // namespace derivatives
