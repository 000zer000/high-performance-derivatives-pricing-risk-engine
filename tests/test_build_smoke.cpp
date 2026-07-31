#include "derivatives/option.hpp"

#include "test_support.hpp"

int main()
{
    constexpr derivatives::EuropeanOption option{
        .spot = 100.0,
        .strike = 100.0,
        .risk_free_rate = 0.05,
        .volatility = 0.20,
        .time_to_maturity = 1.0,
        .type = derivatives::OptionType::Call,
    };

    bool passed = true;
    passed = expect_true(option.spot == 100.0, "spot value is preserved")
        && passed;
    passed = expect_true(
                 option.type == derivatives::OptionType::Call,
                 "option type is preserved"
             )
        && passed;

    return passed ? 0 : 1;
}
