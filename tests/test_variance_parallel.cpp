#include "derivatives/black_scholes.hpp"
#include "derivatives/monte_carlo.hpp"

#include "test_support.hpp"

#include <stdexcept>
#include <string_view>

namespace {

derivatives::EuropeanOption option()
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

bool same_estimate(
    const derivatives::MonteCarloResult& first,
    const derivatives::MonteCarloResult& second
)
{
    return first.price == second.price
        && first.standard_error == second.standard_error
        && first.confidence_interval_lower == second.confidence_interval_lower
        && first.confidence_interval_upper == second.confidence_interval_upper
        && first.effective_samples == second.effective_samples;
}

bool test_invalid_antithetic_path_count()
{
    return expect_throws_containing<std::invalid_argument>(
        [] {
            static_cast<void>(derivatives::price_european_monte_carlo(
                option(),
                {
                    .paths = 999U,
                    .seed = 42U,
                    .variance_reduction =
                        derivatives::VarianceReduction::Antithetic,
                }
            ));
        },
        "even path count",
        "odd antithetic path count"
    );
}

bool test_antithetic_reduces_standard_error()
{
    const auto plain = derivatives::price_european_monte_carlo(
        option(),
        {.paths = 200'000U, .seed = 20'260'731U}
    );
    const auto antithetic = derivatives::price_european_monte_carlo(
        option(),
        {
            .paths = 200'000U,
            .seed = 20'260'731U,
            .variance_reduction = derivatives::VarianceReduction::Antithetic,
        }
    );
    return expect_true(
        antithetic.effective_samples == 100'000U
            && antithetic.standard_error < plain.standard_error,
        "antithetic pairs reduce call standard error on the frozen case"
    );
}

bool test_control_variate_reduces_standard_error()
{
    const auto plain = derivatives::price_european_monte_carlo(
        option(),
        {.paths = 200'000U, .seed = 20'260'731U}
    );
    const auto controlled = derivatives::price_european_monte_carlo(
        option(),
        {
            .paths = 200'000U,
            .seed = 20'260'731U,
            .variance_reduction =
                derivatives::VarianceReduction::ControlVariate,
        }
    );
    const double analytical = derivatives::black_scholes_price(option());
    return expect_true(
               controlled.control_variate_beta > 0.0,
               "analytical stock control beta is positive"
           )
        && expect_true(
            controlled.standard_error < plain.standard_error,
            "control variate reduces call standard error on the frozen case"
        )
        && expect_near(
            controlled.price,
            analytical,
            4.0 * controlled.standard_error,
            "controlled estimate agrees with Black-Scholes"
        );
}

bool test_put_control_variate_agrees_with_reference()
{
    auto put = option();
    put.type = derivatives::OptionType::Put;
    const auto controlled = derivatives::price_european_monte_carlo(
        put,
        {
            .paths = 200'000U,
            .seed = 20'260'731U,
            .variance_reduction =
                derivatives::VarianceReduction::ControlVariate,
        }
    );
    return expect_true(
               controlled.control_variate_beta < 0.0,
               "put payoff has a negative discounted-stock control beta"
           )
        && expect_near(
            controlled.price,
            derivatives::black_scholes_price(put),
            4.0 * controlled.standard_error,
            "controlled put estimate agrees with Black-Scholes"
        );
}

bool test_openmp_unavailable_is_rejected()
{
    if (derivatives::openmp_available()) {
        return true;
    }
    return expect_throws_containing<std::runtime_error>(
        [] {
            static_cast<void>(derivatives::price_european_monte_carlo(
                option(),
                {
                    .paths = 100U,
                    .seed = 42U,
                    .execution = derivatives::ExecutionPolicy::OpenMP,
                    .threads = 2U,
                }
            ));
        },
        "does not include OpenMP",
        "unavailable OpenMP policy"
    );
}

bool test_serial_parallel_exact_match()
{
    if (!derivatives::openmp_available()) {
        return true;
    }
    constexpr std::uint64_t paths = 200'003U;
    const auto serial = derivatives::price_european_monte_carlo(
        option(),
        {.paths = paths, .seed = 20'260'731U}
    );
    const auto parallel = derivatives::price_european_monte_carlo(
        option(),
        {
            .paths = paths,
            .seed = 20'260'731U,
            .execution = derivatives::ExecutionPolicy::OpenMP,
            .threads = 4U,
        }
    );
    return expect_true(
               same_estimate(serial, parallel),
               "fixed blocks make serial and OpenMP estimates identical"
           )
        && expect_true(
            parallel.threads_used == 4U,
            "result records the actual four-thread team"
        );
}

bool test_parallel_asian_exact_match()
{
    if (!derivatives::openmp_available()) {
        return true;
    }
    const derivatives::AsianOption asian{
        .underlying = option(),
        .monitoring_steps = 24U,
    };
    const auto serial = derivatives::price_asian_monte_carlo(
        asian,
        {
            .paths = 80'000U,
            .seed = 42U,
            .variance_reduction = derivatives::VarianceReduction::Antithetic,
        }
    );
    const auto parallel = derivatives::price_asian_monte_carlo(
        asian,
        {
            .paths = 80'000U,
            .seed = 42U,
            .variance_reduction = derivatives::VarianceReduction::Antithetic,
            .execution = derivatives::ExecutionPolicy::OpenMP,
            .threads = 4U,
        }
    );
    return expect_true(
        same_estimate(serial, parallel),
        "path-dependent serial and OpenMP estimates are identical"
    );
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 2;
    }
    const std::string_view name{argv[1]};
    if (name == "invalid_antithetic_path_count") {
        return test_invalid_antithetic_path_count() ? 0 : 1;
    }
    if (name == "antithetic_reduces_standard_error") {
        return test_antithetic_reduces_standard_error() ? 0 : 1;
    }
    if (name == "control_variate_reduces_standard_error") {
        return test_control_variate_reduces_standard_error() ? 0 : 1;
    }
    if (name == "put_control_variate_agrees_with_reference") {
        return test_put_control_variate_agrees_with_reference() ? 0 : 1;
    }
    if (name == "openmp_unavailable_is_rejected") {
        return test_openmp_unavailable_is_rejected() ? 0 : 1;
    }
    if (name == "serial_parallel_exact_match") {
        return test_serial_parallel_exact_match() ? 0 : 1;
    }
    if (name == "parallel_asian_exact_match") {
        return test_parallel_asian_exact_match() ? 0 : 1;
    }
    return 2;
}
