#include "derivatives/monte_carlo.hpp"

#include "derivatives/black_scholes.hpp"
#include "derivatives/running_statistics.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace derivatives {

namespace {

constexpr double confidence_interval_z_95 = 1.96;

void validate(const MonteCarloConfig& config)
{
    if (config.paths < 2U) {
        throw std::invalid_argument("Monte Carlo paths must be at least two");
    }
}

double payoff(const EuropeanOption& option, double terminal_spot)
{
    if (option.type == OptionType::Call) {
        return std::max(terminal_spot - option.strike, 0.0);
    }
    return std::max(option.strike - terminal_spot, 0.0);
}

MonteCarloResult deterministic_result(
    const EuropeanOption& option,
    const MonteCarloConfig& config
)
{
    const double price = black_scholes_price(option);
    return {
        .price = price,
        .standard_error = 0.0,
        .confidence_interval_lower = price,
        .confidence_interval_upper = price,
        .paths = config.paths,
        .seed = config.seed,
    };
}

}  // namespace

MonteCarloResult price_european_monte_carlo(
    const EuropeanOption& option,
    const MonteCarloConfig& config
)
{
    validate(option);
    validate(config);

    if (option.time_to_maturity == 0.0 || option.volatility == 0.0) {
        return deterministic_result(option, config);
    }

    std::mt19937_64 generator{config.seed};
    std::normal_distribution<double> standard_normal{0.0, 1.0};
    RunningStatistics discounted_payoff_statistics;

    const double variance = option.volatility * option.volatility;
    const double drift =
        (option.risk_free_rate - 0.5 * variance) * option.time_to_maturity;
    const double diffusion =
        option.volatility * std::sqrt(option.time_to_maturity);
    const double discount_factor =
        std::exp(-option.risk_free_rate * option.time_to_maturity);

    for (std::uint64_t path = 0U; path < config.paths; ++path) {
        const double normal_draw = standard_normal(generator);
        const double terminal_spot =
            option.spot * std::exp(drift + diffusion * normal_draw);
        discounted_payoff_statistics.add(
            discount_factor * payoff(option, terminal_spot)
        );
    }

    const double price = discounted_payoff_statistics.mean();
    const double standard_error = discounted_payoff_statistics.standard_error();
    const double interval_half_width =
        confidence_interval_z_95 * standard_error;

    return {
        .price = price,
        .standard_error = standard_error,
        .confidence_interval_lower = price - interval_half_width,
        .confidence_interval_upper = price + interval_half_width,
        .paths = config.paths,
        .seed = config.seed,
    };
}

}  // namespace derivatives
