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

/// Arithmetic-average Asian option sampled at equally spaced future times.
struct AsianOption {
    EuropeanOption underlying{};
    std::uint32_t monitoring_steps{};
};

/// Discretely monitored down-and-out option with zero rebate.
struct DownAndOutOption {
    EuropeanOption underlying{};
    double barrier{};
    std::uint32_t monitoring_steps{};
};

/// Validates the underlying inputs and requires at least one monitoring step.
void validate(const AsianOption& option);

/// Also requires a finite, strictly positive barrier.
void validate(const DownAndOutOption& option);

}  // namespace derivatives
