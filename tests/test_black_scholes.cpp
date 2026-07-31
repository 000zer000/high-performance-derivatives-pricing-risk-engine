#include "derivatives/black_scholes.hpp"

#include "test_support.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace {

constexpr double reference_tolerance = 1e-10;

derivatives::EuropeanOption reference_option(derivatives::OptionType type)
{
    return {
        .spot = 100.0,
        .strike = 100.0,
        .risk_free_rate = 0.05,
        .volatility = 0.20,
        .time_to_maturity = 1.0,
        .type = type,
    };
}

bool test_reference_call()
{
    const double price = derivatives::black_scholes_price(
        reference_option(derivatives::OptionType::Call)
    );
    return expect_near(
        price,
        10.450583572185565,
        reference_tolerance,
        "reference European call price"
    );
}

bool test_reference_put()
{
    const double price = derivatives::black_scholes_price(
        reference_option(derivatives::OptionType::Put)
    );
    return expect_near(
        price,
        5.573526022256971,
        reference_tolerance,
        "reference European put price"
    );
}

bool test_put_call_parity()
{
    const auto call_option = reference_option(derivatives::OptionType::Call);
    const auto put_option = reference_option(derivatives::OptionType::Put);
    const double call = derivatives::black_scholes_price(call_option);
    const double put = derivatives::black_scholes_price(put_option);
    const double parity_right_hand_side = call_option.spot
        - call_option.strike
            * std::exp(
                -call_option.risk_free_rate * call_option.time_to_maturity
            );
    return expect_near(
        call - put,
        parity_right_hand_side,
        1e-12,
        "put-call parity"
    );
}

bool test_zero_maturity_call()
{
    auto option = reference_option(derivatives::OptionType::Call);
    option.spot = 120.0;
    option.time_to_maturity = 0.0;
    return expect_near(
        derivatives::black_scholes_price(option),
        20.0,
        0.0,
        "expired call equals intrinsic value"
    );
}

bool test_zero_maturity_put()
{
    auto option = reference_option(derivatives::OptionType::Put);
    option.spot = 80.0;
    option.time_to_maturity = 0.0;
    return expect_near(
        derivatives::black_scholes_price(option),
        20.0,
        0.0,
        "expired put equals intrinsic value"
    );
}

bool test_zero_volatility()
{
    auto call_option = reference_option(derivatives::OptionType::Call);
    call_option.strike = 95.0;
    call_option.risk_free_rate = 0.03;
    call_option.volatility = 0.0;
    call_option.time_to_maturity = 2.0;
    auto put_option = call_option;
    put_option.type = derivatives::OptionType::Put;

    const double discounted_strike = call_option.strike
        * std::exp(
            -call_option.risk_free_rate * call_option.time_to_maturity
        );
    const double expected_call =
        std::max(call_option.spot - discounted_strike, 0.0);
    const double expected_put =
        std::max(discounted_strike - call_option.spot, 0.0);

    bool passed = expect_near(
        derivatives::black_scholes_price(call_option),
        expected_call,
        1e-12,
        "zero-volatility call"
    );
    passed = expect_near(
                 derivatives::black_scholes_price(put_option),
                 expected_put,
                 1e-12,
                 "zero-volatility put"
             )
        && passed;
    return passed;
}

bool test_negative_rate_is_supported()
{
    auto call_option = reference_option(derivatives::OptionType::Call);
    call_option.risk_free_rate = -0.02;
    auto put_option = call_option;
    put_option.type = derivatives::OptionType::Put;

    const double call = derivatives::black_scholes_price(call_option);
    const double put = derivatives::black_scholes_price(put_option);
    const double parity_right_hand_side = call_option.spot
        - call_option.strike
            * std::exp(
                -call_option.risk_free_rate * call_option.time_to_maturity
            );
    return expect_near(
        call - put,
        parity_right_hand_side,
        1e-12,
        "put-call parity with a negative rate"
    );
}

bool test_call_nonnegative()
{
    auto option = reference_option(derivatives::OptionType::Call);
    option.spot = 1.0;
    option.strike = 1'000.0;
    return expect_true(
        derivatives::black_scholes_price(option) >= 0.0,
        "call price is non-negative"
    );
}

bool test_put_nonnegative()
{
    auto option = reference_option(derivatives::OptionType::Put);
    option.spot = 1'000.0;
    option.strike = 1.0;
    return expect_true(
        derivatives::black_scholes_price(option) >= 0.0,
        "put price is non-negative"
    );
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 2;
    }

    const std::string_view test_name{argv[1]};
    if (test_name == "reference_call") {
        return test_reference_call() ? 0 : 1;
    }
    if (test_name == "reference_put") {
        return test_reference_put() ? 0 : 1;
    }
    if (test_name == "put_call_parity") {
        return test_put_call_parity() ? 0 : 1;
    }
    if (test_name == "zero_maturity_call") {
        return test_zero_maturity_call() ? 0 : 1;
    }
    if (test_name == "zero_maturity_put") {
        return test_zero_maturity_put() ? 0 : 1;
    }
    if (test_name == "zero_volatility") {
        return test_zero_volatility() ? 0 : 1;
    }
    if (test_name == "negative_rate_is_supported") {
        return test_negative_rate_is_supported() ? 0 : 1;
    }
    if (test_name == "call_nonnegative") {
        return test_call_nonnegative() ? 0 : 1;
    }
    if (test_name == "put_nonnegative") {
        return test_put_nonnegative() ? 0 : 1;
    }
    return 2;
}
