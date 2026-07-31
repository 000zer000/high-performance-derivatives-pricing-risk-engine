#include "derivatives/black_scholes.hpp"
#include "derivatives/monte_carlo.hpp"

#include "test_support.hpp"

#include <algorithm>
#include <cmath>
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

bool same_result(
    const derivatives::MonteCarloResult& first,
    const derivatives::MonteCarloResult& second
)
{
    return first.price == second.price
        && first.standard_error == second.standard_error
        && first.confidence_interval_lower == second.confidence_interval_lower
        && first.confidence_interval_upper == second.confidence_interval_upper
        && first.paths == second.paths && first.seed == second.seed;
}

bool test_path_count_below_two()
{
    const auto option = reference_option(derivatives::OptionType::Call);
    bool passed = expect_throws_containing<std::invalid_argument>(
        [&option] {
            static_cast<void>(derivatives::price_european_monte_carlo(
                option,
                {.paths = 0U, .seed = 7U}
            ));
        },
        "paths",
        "zero paths are rejected"
    );
    passed = expect_throws_containing<std::invalid_argument>(
                 [&option] {
                     static_cast<void>(
                         derivatives::price_european_monte_carlo(
                             option,
                             {.paths = 1U, .seed = 7U}
                         )
                     );
                 },
                 "paths",
                 "one path is rejected"
             )
        && passed;
    return passed;
}

bool test_same_seed_same_fixed_toolchain_result()
{
    const auto option = reference_option(derivatives::OptionType::Call);
    constexpr derivatives::MonteCarloConfig config{
        .paths = 20'000U,
        .seed = 42U,
    };
    const auto first = derivatives::price_european_monte_carlo(option, config);
    const auto second = derivatives::price_european_monte_carlo(option, config);
    return expect_true(
        same_result(first, second),
        "same seed and fixed toolchain reproduce every result field"
    );
}

bool test_different_seed_changes_stream()
{
    const auto option = reference_option(derivatives::OptionType::Call);
    const auto first = derivatives::price_european_monte_carlo(
        option,
        {.paths = 20'000U, .seed = 42U}
    );
    const auto second = derivatives::price_european_monte_carlo(
        option,
        {.paths = 20'000U, .seed = 43U}
    );
    return expect_true(
        first.price != second.price,
        "different seeds change the sampled estimate"
    );
}

bool test_result_records_seed_and_path_count()
{
    const auto option = reference_option(derivatives::OptionType::Put);
    constexpr derivatives::MonteCarloConfig config{
        .paths = 12'345U,
        .seed = 987'654'321U,
    };
    const auto result = derivatives::price_european_monte_carlo(option, config);
    return expect_true(
        result.paths == config.paths && result.seed == config.seed,
        "result records the requested path count and seed"
    );
}

bool test_confidence_interval_formula()
{
    const auto option = reference_option(derivatives::OptionType::Call);
    const auto result = derivatives::price_european_monte_carlo(
        option,
        {.paths = 20'000U, .seed = 42U}
    );
    const double expected_half_width = 1.96 * result.standard_error;
    bool passed = expect_near(
        result.confidence_interval_lower,
        result.price - expected_half_width,
        1e-14,
        "lower 95 percent interval bound"
    );
    passed = expect_near(
                 result.confidence_interval_upper,
                 result.price + expected_half_width,
                 1e-14,
                 "upper 95 percent interval bound"
             )
        && passed;
    return passed;
}

bool test_zero_maturity_matches_payoff()
{
    auto option = reference_option(derivatives::OptionType::Call);
    option.spot = 120.0;
    option.time_to_maturity = 0.0;
    const auto result = derivatives::price_european_monte_carlo(
        option,
        {.paths = 100U, .seed = 42U}
    );
    bool passed = expect_near(result.price, 20.0, 0.0, "expired payoff");
    passed = expect_near(result.standard_error, 0.0, 0.0, "expired payoff SE")
        && passed;
    passed = expect_near(
                 result.confidence_interval_lower,
                 20.0,
                 0.0,
                 "expired payoff lower bound"
             )
        && passed;
    passed = expect_near(
                 result.confidence_interval_upper,
                 20.0,
                 0.0,
                 "expired payoff upper bound"
             )
        && passed;
    return passed;
}

bool test_zero_volatility_matches_discounted_payoff()
{
    auto option = reference_option(derivatives::OptionType::Call);
    option.strike = 95.0;
    option.risk_free_rate = 0.03;
    option.volatility = 0.0;
    option.time_to_maturity = 2.0;
    const double expected = std::max(
        option.spot
            - option.strike
                * std::exp(-option.risk_free_rate * option.time_to_maturity),
        0.0
    );
    const auto result = derivatives::price_european_monte_carlo(
        option,
        {.paths = 100U, .seed = 42U}
    );
    return expect_near(result.price, expected, 1e-12, "zero-volatility price")
        && expect_near(
            result.standard_error,
            0.0,
            0.0,
            "zero-volatility standard error"
        );
}

bool estimate_within_four_standard_errors(derivatives::OptionType type)
{
    const auto option = reference_option(type);
    const auto result = derivatives::price_european_monte_carlo(
        option,
        {.paths = 250'000U, .seed = 20'260'731U}
    );
    const double analytical = derivatives::black_scholes_price(option);
    return expect_near(
        result.price,
        analytical,
        4.0 * result.standard_error,
        "fixed-seed estimate is within four reported standard errors"
    );
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 2;
    }

    const std::string_view test_name{argv[1]};
    if (test_name == "path_count_below_two") {
        return test_path_count_below_two() ? 0 : 1;
    }
    if (test_name == "same_seed_same_fixed_toolchain_result") {
        return test_same_seed_same_fixed_toolchain_result() ? 0 : 1;
    }
    if (test_name == "different_seed_changes_stream") {
        return test_different_seed_changes_stream() ? 0 : 1;
    }
    if (test_name == "result_records_seed_and_path_count") {
        return test_result_records_seed_and_path_count() ? 0 : 1;
    }
    if (test_name == "confidence_interval_formula") {
        return test_confidence_interval_formula() ? 0 : 1;
    }
    if (test_name == "zero_maturity_matches_payoff") {
        return test_zero_maturity_matches_payoff() ? 0 : 1;
    }
    if (test_name == "zero_volatility_matches_discounted_payoff") {
        return test_zero_volatility_matches_discounted_payoff() ? 0 : 1;
    }
    if (test_name == "call_estimate_within_four_standard_errors") {
        return estimate_within_four_standard_errors(
                   derivatives::OptionType::Call
               )
            ? 0
            : 1;
    }
    if (test_name == "put_estimate_within_four_standard_errors") {
        return estimate_within_four_standard_errors(
                   derivatives::OptionType::Put
               )
            ? 0
            : 1;
    }
    return 2;
}
