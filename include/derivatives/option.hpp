#pragma once

#include <cstdint>

namespace derivatives {

enum class OptionType : std::uint8_t {
    Call,
    Put,
};

struct EuropeanOption {
    double spot{};
    double strike{};
    double risk_free_rate{};
    double volatility{};
    double time_to_maturity{};
    OptionType type{OptionType::Call};
};

void validate(const EuropeanOption& option);

}  // namespace derivatives
