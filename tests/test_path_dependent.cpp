#include "derivatives/monte_carlo.hpp"

#include "test_support.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

derivatives::EuropeanOption underlying()
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

bool same_numerical_result(
    const derivatives::MonteCarloResult& first,
    const derivatives::MonteCarloResult& second
)
{
    return first.price == second.price
        && first.standard_error == second.standard_error
        && first.confidence_interval_lower == second.confidence_interval_lower
        && first.confidence_interval_upper == second.confidence_interval_upper;
}

bool test_invalid_contracts()
{
    derivatives::AsianOption asian{
        .underlying = underlying(),
        .monitoring_steps = 0U,
    };
    bool passed = expect_throws_containing<std::invalid_argument>(
        [&asian] { derivatives::validate(asian); },
        "monitoring_steps",
        "Asian monitoring steps"
    );
    derivatives::DownAndOutOption barrier{
        .underlying = underlying(),
        .barrier = 0.0,
        .monitoring_steps = 12U,
    };
    passed = expect_throws_containing<std::invalid_argument>(
                 [&barrier] { derivatives::validate(barrier); },
                 "barrier",
                 "non-positive barrier"
             )
        && passed;
    barrier.barrier = std::numeric_limits<double>::infinity();
    return expect_throws_containing<std::invalid_argument>(
               [&barrier] { derivatives::validate(barrier); },
               "finite",
               "non-finite barrier"
           )
        && passed;
}

bool test_asian_same_seed()
{
    const derivatives::AsianOption option{
        .underlying = underlying(),
        .monitoring_steps = 12U,
    };
    constexpr derivatives::MonteCarloConfig config{
        .paths = 40'000U,
        .seed = 20'260'731U,
    };
    return expect_true(
        same_numerical_result(
            derivatives::price_asian_monte_carlo(option, config),
            derivatives::price_asian_monte_carlo(option, config)
        ),
        "Asian pricing is deterministic for a fixed configuration"
    );
}

bool test_asian_zero_volatility()
{
    auto base = underlying();
    base.volatility = 0.0;
    constexpr std::uint32_t steps = 4U;
    const derivatives::AsianOption option{
        .underlying = base,
        .monitoring_steps = steps,
    };
    const double time_step =
        base.time_to_maturity / static_cast<double>(steps);
    double average = 0.0;
    for (std::uint32_t step = 1U; step <= steps; ++step) {
        average += base.spot
            * std::exp(
                base.risk_free_rate * time_step * static_cast<double>(step)
            );
    }
    average /= static_cast<double>(steps);
    const double expected = std::exp(
                                -base.risk_free_rate
                                * base.time_to_maturity
                            )
        * std::max(average - base.strike, 0.0);
    const auto result = derivatives::price_asian_monte_carlo(
        option,
        {.paths = 100U, .seed = 42U}
    );
    return expect_near(result.price, expected, 1e-12, "deterministic Asian")
        && expect_near(result.standard_error, 0.0, 0.0, "deterministic SE");
}

bool test_barrier_immediate_knockout()
{
    const derivatives::DownAndOutOption option{
        .underlying = underlying(),
        .barrier = 100.0,
        .monitoring_steps = 12U,
    };
    const auto result = derivatives::price_down_and_out_monte_carlo(
        option,
        {.paths = 10'000U, .seed = 42U}
    );
    return expect_near(result.price, 0.0, 0.0, "initial barrier knockout")
        && expect_near(result.standard_error, 0.0, 0.0, "knockout SE");
}

bool test_barrier_deterministic_crossing()
{
    auto base = underlying();
    base.risk_free_rate = -0.10;
    base.volatility = 0.0;
    const derivatives::DownAndOutOption option{
        .underlying = base,
        .barrier = 95.0,
        .monitoring_steps = 12U,
    };
    const auto result = derivatives::price_down_and_out_monte_carlo(
        option,
        {.paths = 100U, .seed = 42U}
    );
    return expect_near(result.price, 0.0, 0.0, "deterministic crossing");
}

bool test_path_dependent_antithetic()
{
    const derivatives::AsianOption option{
        .underlying = underlying(),
        .monitoring_steps = 12U,
    };
    const auto result = derivatives::price_asian_monte_carlo(
        option,
        {
            .paths = 40'000U,
            .seed = 42U,
            .variance_reduction = derivatives::VarianceReduction::Antithetic,
        }
    );
    return expect_true(
        result.effective_samples == 20'000U && result.standard_error > 0.0,
        "antithetic Asian estimator counts independent pairs"
    );
}

bool test_control_variate_rejected()
{
    const derivatives::AsianOption option{
        .underlying = underlying(),
        .monitoring_steps = 12U,
    };
    return expect_throws_containing<std::invalid_argument>(
        [&option] {
            static_cast<void>(derivatives::price_asian_monte_carlo(
                option,
                {
                    .paths = 100U,
                    .seed = 42U,
                    .variance_reduction =
                        derivatives::VarianceReduction::ControlVariate,
                }
            ));
        },
        "only for European",
        "unsupported path-dependent control variate"
    );
}

bool test_control_variate_rejected_for_deterministic_contract()
{
    auto base = underlying();
    base.volatility = 0.0;
    const derivatives::AsianOption option{
        .underlying = base,
        .monitoring_steps = 12U,
    };
    return expect_throws_containing<std::invalid_argument>(
        [&option] {
            static_cast<void>(derivatives::price_asian_monte_carlo(
                option,
                {
                    .paths = 100U,
                    .seed = 42U,
                    .variance_reduction =
                        derivatives::VarianceReduction::ControlVariate,
                }
            ));
        },
        "only for European",
        "unsupported control variate is rejected before deterministic shortcut"
    );
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 2;
    }
    const std::string_view name{argv[1]};
    if (name == "invalid_contracts") {
        return test_invalid_contracts() ? 0 : 1;
    }
    if (name == "asian_same_seed") {
        return test_asian_same_seed() ? 0 : 1;
    }
    if (name == "asian_zero_volatility") {
        return test_asian_zero_volatility() ? 0 : 1;
    }
    if (name == "barrier_immediate_knockout") {
        return test_barrier_immediate_knockout() ? 0 : 1;
    }
    if (name == "barrier_deterministic_crossing") {
        return test_barrier_deterministic_crossing() ? 0 : 1;
    }
    if (name == "path_dependent_antithetic") {
        return test_path_dependent_antithetic() ? 0 : 1;
    }
    if (name == "control_variate_rejected") {
        return test_control_variate_rejected() ? 0 : 1;
    }
    if (name == "control_variate_rejected_for_deterministic_contract") {
        return test_control_variate_rejected_for_deterministic_contract()
            ? 0
            : 1;
    }
    return 2;
}
