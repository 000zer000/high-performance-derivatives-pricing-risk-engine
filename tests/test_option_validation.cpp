#include "derivatives/option.hpp"

#include "test_support.hpp"

#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

derivatives::EuropeanOption valid_option()
{
    return {
        .spot = 100.0,
        .strike = 100.0,
        .risk_free_rate = 0.05,
        .volatility = 0.20,
        .time_to_maturity = 1.0,
        .type = derivatives::OptionType::Call,
    };
}

bool rejected(
    const derivatives::EuropeanOption& option,
    std::string_view field,
    std::string_view message
)
{
    return expect_throws_containing<std::invalid_argument>(
        [&option] { derivatives::validate(option); },
        field,
        message
    );
}

bool test_spot_non_positive()
{
    auto zero = valid_option();
    zero.spot = 0.0;
    auto negative = valid_option();
    negative.spot = -1.0;
    return rejected(zero, "spot", "zero spot is rejected")
        && rejected(negative, "spot", "negative spot is rejected");
}

bool test_strike_non_positive()
{
    auto zero = valid_option();
    zero.strike = 0.0;
    auto negative = valid_option();
    negative.strike = -1.0;
    return rejected(zero, "strike", "zero strike is rejected")
        && rejected(negative, "strike", "negative strike is rejected");
}

bool test_negative_volatility()
{
    auto option = valid_option();
    option.volatility = -0.01;
    return rejected(option, "volatility", "negative volatility is rejected");
}

bool test_negative_maturity()
{
    auto option = valid_option();
    option.time_to_maturity = -0.01;
    return rejected(option, "time_to_maturity", "negative maturity is rejected");
}

bool test_non_finite_inputs()
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    bool passed = true;

    for (const double invalid_value : {nan, infinity}) {
        auto spot = valid_option();
        spot.spot = invalid_value;
        passed = rejected(spot, "spot", "non-finite spot is rejected") && passed;

        auto strike = valid_option();
        strike.strike = invalid_value;
        passed = rejected(strike, "strike", "non-finite strike is rejected")
            && passed;

        auto rate = valid_option();
        rate.risk_free_rate = invalid_value;
        passed = rejected(rate, "risk_free_rate", "non-finite rate is rejected")
            && passed;

        auto volatility = valid_option();
        volatility.volatility = invalid_value;
        passed = rejected(
                     volatility,
                     "volatility",
                     "non-finite volatility is rejected"
                 )
            && passed;

        auto maturity = valid_option();
        maturity.time_to_maturity = invalid_value;
        passed = rejected(
                     maturity,
                     "time_to_maturity",
                     "non-finite maturity is rejected"
                 )
            && passed;
    }
    return passed;
}

bool test_negative_rate_is_valid()
{
    auto option = valid_option();
    option.risk_free_rate = -0.01;
    derivatives::validate(option);
    return true;
}

bool test_invalid_option_type()
{
    auto option = valid_option();
    option.type = static_cast<derivatives::OptionType>(255U);
    return rejected(option, "option type", "invalid option type is rejected");
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 2;
    }

    const std::string_view test_name{argv[1]};
    if (test_name == "spot_non_positive") {
        return test_spot_non_positive() ? 0 : 1;
    }
    if (test_name == "strike_non_positive") {
        return test_strike_non_positive() ? 0 : 1;
    }
    if (test_name == "negative_volatility") {
        return test_negative_volatility() ? 0 : 1;
    }
    if (test_name == "negative_maturity") {
        return test_negative_maturity() ? 0 : 1;
    }
    if (test_name == "non_finite_inputs") {
        return test_non_finite_inputs() ? 0 : 1;
    }
    if (test_name == "negative_rate_is_valid") {
        return test_negative_rate_is_valid() ? 0 : 1;
    }
    if (test_name == "invalid_option_type") {
        return test_invalid_option_type() ? 0 : 1;
    }
    return 2;
}
