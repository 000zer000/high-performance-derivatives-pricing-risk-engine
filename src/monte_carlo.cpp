#include "derivatives/monte_carlo.hpp"

#include "derivatives/black_scholes.hpp"
#include "derivatives/running_statistics.hpp"
#include "deterministic_random.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#ifdef DPR_HAS_OPENMP
#include <omp.h>
#endif

namespace derivatives {

namespace {

constexpr double confidence_interval_z_95 = 1.96;
constexpr std::uint64_t samples_per_block = 2'048U;

struct SimulationOutput {
    RunningStatistics statistics{};
    std::uint32_t threads_used{1U};
};

double payoff(OptionType type, double underlying, double strike)
{
    if (type == OptionType::Call) {
        return std::max(underlying - strike, 0.0);
    }
    return std::max(strike - underlying, 0.0);
}

std::uint64_t effective_sample_count(const MonteCarloConfig& config)
{
    return config.variance_reduction == VarianceReduction::Antithetic
        ? config.paths / 2U
        : config.paths;
}

void validate(const MonteCarloConfig& config)
{
    if (config.paths < 2U) {
        throw std::invalid_argument("Monte Carlo paths must be at least two");
    }
    if (config.variance_reduction != VarianceReduction::None
        && config.variance_reduction != VarianceReduction::Antithetic
        && config.variance_reduction != VarianceReduction::ControlVariate) {
        throw std::invalid_argument("unknown variance-reduction method");
    }
    if (config.variance_reduction == VarianceReduction::Antithetic
        && (config.paths < 4U || config.paths % 2U != 0U)) {
        throw std::invalid_argument(
            "antithetic sampling requires an even path count of at least four"
        );
    }
    if (config.execution != ExecutionPolicy::Serial
        && config.execution != ExecutionPolicy::OpenMP) {
        throw std::invalid_argument("unknown execution policy");
    }
    if (config.execution == ExecutionPolicy::Serial && config.threads != 0U) {
        throw std::invalid_argument(
            "threads may be set only for OpenMP execution"
        );
    }
    if (config.threads
        > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("thread count exceeds the OpenMP limit");
    }
#ifndef DPR_HAS_OPENMP
    if (config.execution == ExecutionPolicy::OpenMP) {
        throw std::runtime_error("this build does not include OpenMP support");
    }
#endif
}

template <typename Evaluator>
SimulationOutput simulate_samples(
    std::uint64_t sample_count,
    const MonteCarloConfig& config,
    Evaluator evaluator
)
{
    const std::uint64_t block_count =
        (sample_count + samples_per_block - 1U) / samples_per_block;
    std::vector<RunningStatistics> blocks(
        static_cast<std::size_t>(block_count)
    );
    std::vector<std::exception_ptr> failures(
        static_cast<std::size_t>(block_count)
    );

    const auto process_block = [&](std::uint64_t block) {
        try {
            const std::uint64_t begin = block * samples_per_block;
            const std::uint64_t end =
                std::min(begin + samples_per_block, sample_count);
            RunningStatistics local;
            for (std::uint64_t sample = begin; sample < end; ++sample) {
                local.add(evaluator(sample));
            }
            blocks.at(static_cast<std::size_t>(block)) = local;
        } catch (...) {
            failures.at(static_cast<std::size_t>(block)) =
                std::current_exception();
        }
    };

    std::uint32_t threads_used = 1U;
    if (config.execution == ExecutionPolicy::OpenMP) {
#ifdef DPR_HAS_OPENMP
        const int requested_threads = config.threads == 0U
            ? omp_get_max_threads()
            : static_cast<int>(config.threads);
        const std::int64_t signed_block_count =
            static_cast<std::int64_t>(block_count);
#pragma omp parallel num_threads(requested_threads)
        {
#pragma omp single
            threads_used = static_cast<std::uint32_t>(omp_get_num_threads());
#pragma omp for schedule(static)
            for (std::int64_t block = 0; block < signed_block_count; ++block) {
                process_block(static_cast<std::uint64_t>(block));
            }
        }
#endif
    } else {
        for (std::uint64_t block = 0U; block < block_count; ++block) {
            process_block(block);
        }
    }

    RunningStatistics combined;
    for (std::uint64_t block = 0U; block < block_count; ++block) {
        const auto& failure = failures.at(static_cast<std::size_t>(block));
        if (failure) {
            std::rethrow_exception(failure);
        }
        combined.merge(blocks.at(static_cast<std::size_t>(block)));
    }
    return {.statistics = combined, .threads_used = threads_used};
}

MonteCarloResult make_result(
    const SimulationOutput& output,
    const MonteCarloConfig& config,
    double control_variate_beta
)
{
    const double price = output.statistics.mean();
    const double standard_error = output.statistics.standard_error();
    const double interval_half_width =
        confidence_interval_z_95 * standard_error;
    return {
        .price = price,
        .standard_error = standard_error,
        .confidence_interval_lower = price - interval_half_width,
        .confidence_interval_upper = price + interval_half_width,
        .paths = config.paths,
        .seed = config.seed,
        .effective_samples = output.statistics.count(),
        .threads_used = output.threads_used,
        .control_variate_beta = control_variate_beta,
    };
}

MonteCarloResult deterministic_result(
    double price,
    const MonteCarloConfig& config
)
{
    return {
        .price = price,
        .standard_error = 0.0,
        .confidence_interval_lower = price,
        .confidence_interval_upper = price,
        .paths = config.paths,
        .seed = config.seed,
        .effective_samples = effective_sample_count(config),
        .threads_used = 1U,
        .control_variate_beta = 0.0,
    };
}

double terminal_spot(
    const EuropeanOption& option,
    std::uint64_t seed,
    std::uint64_t path,
    double normal_sign
)
{
    const double variance = option.volatility * option.volatility;
    const double drift =
        (option.risk_free_rate - 0.5 * variance)
        * option.time_to_maturity;
    const double diffusion =
        option.volatility * std::sqrt(option.time_to_maturity);
    return option.spot
        * std::exp(
            drift
            + diffusion * normal_sign * detail::normal_draw(seed, path, 0U)
        );
}

double discounted_european_payoff(
    const EuropeanOption& option,
    std::uint64_t seed,
    std::uint64_t path,
    double normal_sign
)
{
    const double discount_factor =
        std::exp(-option.risk_free_rate * option.time_to_maturity);
    return discount_factor
        * payoff(
            option.type,
            terminal_spot(option, seed, path, normal_sign),
            option.strike
        );
}

double standard_normal_cdf(double value)
{
    return 0.5 * std::erfc(-value / std::sqrt(2.0));
}

double european_stock_control_beta(const EuropeanOption& option)
{
    const double variance_horizon =
        option.volatility * option.volatility * option.time_to_maturity;
    const double control_variance =
        option.spot * option.spot * std::expm1(variance_horizon);
    const double volatility_horizon = std::sqrt(variance_horizon);
    const double d1 =
        (std::log(option.spot) - std::log(option.strike)
         + (option.risk_free_rate
            + 0.5 * option.volatility * option.volatility)
             * option.time_to_maturity)
        / volatility_horizon;
    const double discount_factor =
        std::exp(-option.risk_free_rate * option.time_to_maturity);
    const double price = black_scholes_price(option);

    double payoff_times_control_expectation{};
    if (option.type == OptionType::Call) {
        payoff_times_control_expectation =
            option.spot * option.spot * std::exp(variance_horizon)
                * standard_normal_cdf(d1 + volatility_horizon)
            - option.strike * option.spot * discount_factor
                * standard_normal_cdf(d1);
    } else {
        payoff_times_control_expectation =
            option.strike * option.spot * discount_factor
                * standard_normal_cdf(-d1)
            - option.spot * option.spot * std::exp(variance_horizon)
                * standard_normal_cdf(-d1 - volatility_horizon);
    }
    const double covariance =
        payoff_times_control_expectation - price * option.spot;
    return covariance / control_variance;
}

double discounted_asian_payoff(
    const AsianOption& option,
    std::uint64_t seed,
    std::uint64_t path,
    double normal_sign
)
{
    const auto& underlying = option.underlying;
    const double time_step = underlying.time_to_maturity
        / static_cast<double>(option.monitoring_steps);
    const double drift_step =
        (underlying.risk_free_rate
         - 0.5 * underlying.volatility * underlying.volatility)
        * time_step;
    const double diffusion_step =
        underlying.volatility * std::sqrt(time_step);
    double simulated_spot = underlying.spot;
    double sum = 0.0;
    for (std::uint32_t step = 0U; step < option.monitoring_steps; ++step) {
        simulated_spot *= std::exp(
            drift_step
            + diffusion_step * normal_sign
                * detail::normal_draw(seed, path, step)
        );
        sum += simulated_spot;
    }
    const double average =
        sum / static_cast<double>(option.monitoring_steps);
    const double discount_factor = std::exp(
        -underlying.risk_free_rate * underlying.time_to_maturity
    );
    return discount_factor
        * payoff(underlying.type, average, underlying.strike);
}

double discounted_down_and_out_payoff(
    const DownAndOutOption& option,
    std::uint64_t seed,
    std::uint64_t path,
    double normal_sign
)
{
    const auto& underlying = option.underlying;
    if (underlying.spot <= option.barrier) {
        return 0.0;
    }
    const double time_step = underlying.time_to_maturity
        / static_cast<double>(option.monitoring_steps);
    const double drift_step =
        (underlying.risk_free_rate
         - 0.5 * underlying.volatility * underlying.volatility)
        * time_step;
    const double diffusion_step =
        underlying.volatility * std::sqrt(time_step);
    double simulated_spot = underlying.spot;
    for (std::uint32_t step = 0U; step < option.monitoring_steps; ++step) {
        simulated_spot *= std::exp(
            drift_step
            + diffusion_step * normal_sign
                * detail::normal_draw(seed, path, step)
        );
        if (simulated_spot <= option.barrier) {
            return 0.0;
        }
    }
    const double discount_factor = std::exp(
        -underlying.risk_free_rate * underlying.time_to_maturity
    );
    return discount_factor
        * payoff(underlying.type, simulated_spot, underlying.strike);
}

double deterministic_asian_price(const AsianOption& option)
{
    const auto& underlying = option.underlying;
    if (underlying.time_to_maturity == 0.0) {
        return payoff(underlying.type, underlying.spot, underlying.strike);
    }
    const double time_step = underlying.time_to_maturity
        / static_cast<double>(option.monitoring_steps);
    double sum = 0.0;
    for (std::uint32_t step = 1U; step <= option.monitoring_steps; ++step) {
        sum += underlying.spot
            * std::exp(
                underlying.risk_free_rate * time_step
                * static_cast<double>(step)
            );
    }
    const double average =
        sum / static_cast<double>(option.monitoring_steps);
    return std::exp(
               -underlying.risk_free_rate * underlying.time_to_maturity
           )
        * payoff(underlying.type, average, underlying.strike);
}

double deterministic_down_and_out_price(const DownAndOutOption& option)
{
    const auto& underlying = option.underlying;
    if (underlying.spot <= option.barrier) {
        return 0.0;
    }
    if (underlying.time_to_maturity == 0.0) {
        return payoff(underlying.type, underlying.spot, underlying.strike);
    }
    const double time_step = underlying.time_to_maturity
        / static_cast<double>(option.monitoring_steps);
    double simulated_spot = underlying.spot;
    for (std::uint32_t step = 0U; step < option.monitoring_steps; ++step) {
        simulated_spot *= std::exp(underlying.risk_free_rate * time_step);
        if (simulated_spot <= option.barrier) {
            return 0.0;
        }
    }
    return std::exp(
               -underlying.risk_free_rate * underlying.time_to_maturity
           )
        * payoff(underlying.type, simulated_spot, underlying.strike);
}

template <typename PayoffEvaluator>
MonteCarloResult price_path_dependent(
    const MonteCarloConfig& config,
    PayoffEvaluator evaluator
)
{
    if (config.variance_reduction == VarianceReduction::ControlVariate) {
        throw std::invalid_argument(
            "the stock control variate is implemented only for European options"
        );
    }
    const std::uint64_t sample_count = effective_sample_count(config);
    const auto sample = [&](std::uint64_t index) {
        const double first = evaluator(index, 1.0);
        if (config.variance_reduction == VarianceReduction::Antithetic) {
            return 0.5 * (first + evaluator(index, -1.0));
        }
        return first;
    };
    return make_result(
        simulate_samples(sample_count, config, sample),
        config,
        0.0
    );
}

}  // namespace

bool openmp_available() noexcept
{
#ifdef DPR_HAS_OPENMP
    return true;
#else
    return false;
#endif
}

MonteCarloResult price_european_monte_carlo(
    const EuropeanOption& option,
    const MonteCarloConfig& config
)
{
    validate(option);
    validate(config);
    if (option.time_to_maturity == 0.0 || option.volatility == 0.0) {
        return deterministic_result(black_scholes_price(option), config);
    }

    const double beta = config.variance_reduction
            == VarianceReduction::ControlVariate
        ? european_stock_control_beta(option)
        : 0.0;
    const double discount_factor =
        std::exp(-option.risk_free_rate * option.time_to_maturity);
    const std::uint64_t sample_count = effective_sample_count(config);
    const auto sample = [&](std::uint64_t index) {
        const double first = discounted_european_payoff(
            option,
            config.seed,
            index,
            1.0
        );
        if (config.variance_reduction == VarianceReduction::Antithetic) {
            const double second = discounted_european_payoff(
                option,
                config.seed,
                index,
                -1.0
            );
            return 0.5 * (first + second);
        }
        if (config.variance_reduction == VarianceReduction::ControlVariate) {
            const double discounted_spot = discount_factor
                * terminal_spot(option, config.seed, index, 1.0);
            return first - beta * (discounted_spot - option.spot);
        }
        return first;
    };
    return make_result(
        simulate_samples(sample_count, config, sample),
        config,
        beta
    );
}

MonteCarloResult price_asian_monte_carlo(
    const AsianOption& option,
    const MonteCarloConfig& config
)
{
    validate(option);
    validate(config);
    if (config.variance_reduction == VarianceReduction::ControlVariate) {
        throw std::invalid_argument(
            "the stock control variate is implemented only for European options"
        );
    }
    if (option.underlying.time_to_maturity == 0.0
        || option.underlying.volatility == 0.0) {
        return deterministic_result(deterministic_asian_price(option), config);
    }
    const auto evaluator = [&](std::uint64_t path, double sign) {
        return discounted_asian_payoff(option, config.seed, path, sign);
    };
    return price_path_dependent(config, evaluator);
}

MonteCarloResult price_down_and_out_monte_carlo(
    const DownAndOutOption& option,
    const MonteCarloConfig& config
)
{
    validate(option);
    validate(config);
    if (config.variance_reduction == VarianceReduction::ControlVariate) {
        throw std::invalid_argument(
            "the stock control variate is implemented only for European options"
        );
    }
    if (option.underlying.time_to_maturity == 0.0
        || option.underlying.volatility == 0.0
        || option.underlying.spot <= option.barrier) {
        return deterministic_result(
            deterministic_down_and_out_price(option),
            config
        );
    }
    const auto evaluator = [&](std::uint64_t path, double sign) {
        return discounted_down_and_out_payoff(option, config.seed, path, sign);
    };
    return price_path_dependent(config, evaluator);
}

}  // namespace derivatives
