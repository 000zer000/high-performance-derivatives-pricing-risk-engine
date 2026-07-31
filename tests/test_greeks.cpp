#include "derivatives/black_scholes.hpp"
#include "derivatives/greeks.hpp"

#include "test_support.hpp"

#include <stdexcept>
#include <string_view>

namespace {

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
    const auto greeks = derivatives::black_scholes_greeks(
        reference_option(derivatives::OptionType::Call)
    );
    return expect_near(greeks.delta, 0.6368306511756191, 1e-12, "call delta")
        && expect_near(
            greeks.gamma,
            0.018762017345846895,
            1e-12,
            "call gamma"
        )
        && expect_near(
            greeks.vega,
            37.52403469169379,
            1e-11,
            "call vega"
        );
}

bool test_reference_put()
{
    const auto greeks = derivatives::black_scholes_greeks(
        reference_option(derivatives::OptionType::Put)
    );
    return expect_near(
               greeks.delta,
               -0.3631693488243809,
               1e-12,
               "put delta"
           )
        && expect_near(
            greeks.gamma,
            0.018762017345846895,
            1e-12,
            "put gamma"
        )
        && expect_near(
            greeks.vega,
            37.52403469169379,
            1e-11,
            "put vega"
        );
}

bool test_call_put_identities()
{
    const auto call = derivatives::black_scholes_greeks(
        reference_option(derivatives::OptionType::Call)
    );
    const auto put = derivatives::black_scholes_greeks(
        reference_option(derivatives::OptionType::Put)
    );
    return expect_near(call.delta - put.delta, 1.0, 1e-14, "delta parity")
        && expect_near(call.gamma, put.gamma, 0.0, "gamma equality")
        && expect_near(call.vega, put.vega, 0.0, "vega equality");
}

bool test_finite_differences()
{
    auto option = reference_option(derivatives::OptionType::Call);
    const auto greeks = derivatives::black_scholes_greeks(option);
    constexpr double spot_step = 0.01;
    const double base_price = derivatives::black_scholes_price(option);
    option.spot += spot_step;
    const double price_up = derivatives::black_scholes_price(option);
    option.spot -= 2.0 * spot_step;
    const double price_down = derivatives::black_scholes_price(option);
    const double finite_delta = (price_up - price_down) / (2.0 * spot_step);
    const double finite_gamma =
        (price_up - 2.0 * base_price + price_down)
        / (spot_step * spot_step);

    option = reference_option(derivatives::OptionType::Call);
    constexpr double volatility_step = 1e-5;
    option.volatility += volatility_step;
    const double volatility_up = derivatives::black_scholes_price(option);
    option.volatility -= 2.0 * volatility_step;
    const double volatility_down = derivatives::black_scholes_price(option);
    const double finite_vega =
        (volatility_up - volatility_down) / (2.0 * volatility_step);

    return expect_near(greeks.delta, finite_delta, 1e-8, "finite delta")
        && expect_near(greeks.gamma, finite_gamma, 1e-7, "finite gamma")
        && expect_near(greeks.vega, finite_vega, 1e-7, "finite vega");
}

bool test_degenerate_inputs_rejected()
{
    auto option = reference_option(derivatives::OptionType::Call);
    option.time_to_maturity = 0.0;
    bool passed = expect_throws_containing<std::domain_error>(
        [&option] {
            static_cast<void>(derivatives::black_scholes_greeks(option));
        },
        "positive maturity",
        "zero maturity Greeks"
    );
    option = reference_option(derivatives::OptionType::Call);
    option.volatility = 0.0;
    return expect_throws_containing<std::domain_error>(
               [&option] {
                   static_cast<void>(
                       derivatives::black_scholes_greeks(option)
                   );
               },
               "positive maturity",
               "zero volatility Greeks"
           )
        && passed;
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 2;
    }
    const std::string_view name{argv[1]};
    if (name == "reference_call") {
        return test_reference_call() ? 0 : 1;
    }
    if (name == "reference_put") {
        return test_reference_put() ? 0 : 1;
    }
    if (name == "call_put_identities") {
        return test_call_put_identities() ? 0 : 1;
    }
    if (name == "finite_differences") {
        return test_finite_differences() ? 0 : 1;
    }
    if (name == "degenerate_inputs_rejected") {
        return test_degenerate_inputs_rejected() ? 0 : 1;
    }
    return 2;
}
