#pragma once

#include <cstdint>

namespace derivatives {

enum class OptionType : std::uint8_t {
    Call,
    Put,
};

/// Inputs for a no-dividend European option under constant rate and volatility.
struct EuropeanOption {
    double spot{};
    double strike{};
    double risk_free_rate{};
    double volatility{};
    double time_to_maturity{};
    OptionType type{OptionType::Call};
};

/// Throws std::invalid_argument when an input is non-finite or outside its domain.
void validate(const EuropeanOption& option);

}  // namespace derivatives
