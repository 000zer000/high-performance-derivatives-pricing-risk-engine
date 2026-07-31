#include "derivatives/monte_carlo.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct Settings {
    std::uint64_t european_paths{5'000'000U};
    std::uint64_t asian_paths{300'000U};
    std::uint32_t asian_steps{64U};
    std::uint32_t repetitions{5U};
    std::uint32_t threads{8U};
};

std::uint64_t parse_unsigned(std::string_view text, std::string_view name)
{
    std::uint64_t value{};
    const auto [position, error] = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value
    );
    if (error != std::errc{} || position != text.data() + text.size()
        || value == 0U) {
        throw std::invalid_argument(std::string{name} + " must be positive");
    }
    return value;
}

std::uint32_t narrow_unsigned(std::uint64_t value, std::string_view name)
{
    if (value > static_cast<std::uint64_t>(UINT32_MAX)) {
        throw std::invalid_argument(std::string{name} + " is too large");
    }
    return static_cast<std::uint32_t>(value);
}

Settings parse_settings(int argc, char* argv[])
{
    if (argc == 1) {
        return {};
    }
    if (argc != 6) {
        throw std::invalid_argument(
            "expected: european_paths asian_paths asian_steps repetitions threads"
        );
    }
    return {
        .european_paths = parse_unsigned(argv[1], "european_paths"),
        .asian_paths = parse_unsigned(argv[2], "asian_paths"),
        .asian_steps = narrow_unsigned(
            parse_unsigned(argv[3], "asian_steps"),
            "asian_steps"
        ),
        .repetitions = narrow_unsigned(
            parse_unsigned(argv[4], "repetitions"),
            "repetitions"
        ),
        .threads = narrow_unsigned(
            parse_unsigned(argv[5], "threads"),
            "threads"
        ),
    };
}

derivatives::EuropeanOption reference_option()
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

template <typename Function>
std::pair<double, derivatives::MonteCarloResult> timed(Function function)
{
    const auto start = std::chrono::steady_clock::now();
    const auto result = function();
    const auto stop = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = stop - start;
    return {elapsed.count(), result};
}

double median(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2U;
    if (values.size() % 2U == 1U) {
        return values.at(middle);
    }
    return 0.5 * (values.at(middle - 1U) + values.at(middle));
}

void print_times(std::string_view name, const std::vector<double>& values)
{
    std::cout << name << '=';
    for (std::size_t index = 0U; index < values.size(); ++index) {
        if (index != 0U) {
            std::cout << ',';
        }
        std::cout << values.at(index);
    }
    std::cout << '\n';
}

bool same_numerical_result(
    const derivatives::MonteCarloResult& first,
    const derivatives::MonteCarloResult& second
)
{
    return first.price == second.price
        && first.standard_error == second.standard_error
        && first.confidence_interval_lower
            == second.confidence_interval_lower
        && first.confidence_interval_upper
            == second.confidence_interval_upper;
}

template <typename SerialFunction, typename ParallelFunction>
void benchmark_pair(
    std::string_view name,
    std::uint32_t repetitions,
    SerialFunction serial_function,
    ParallelFunction parallel_function
)
{
    std::vector<double> serial_seconds;
    std::vector<double> parallel_seconds;
    serial_seconds.reserve(repetitions);
    parallel_seconds.reserve(repetitions);
    derivatives::MonteCarloResult serial_result;
    derivatives::MonteCarloResult parallel_result;
    bool every_result_equal = true;

    static_cast<void>(serial_function());
    static_cast<void>(parallel_function());

    for (std::uint32_t repetition = 0U; repetition < repetitions; ++repetition) {
        if (repetition % 2U == 0U) {
            auto [serial_time, first] = timed(serial_function);
            auto [parallel_time, second] = timed(parallel_function);
            serial_seconds.push_back(serial_time);
            parallel_seconds.push_back(parallel_time);
            serial_result = first;
            parallel_result = second;
            every_result_equal = every_result_equal
                && same_numerical_result(first, second);
        } else {
            auto [parallel_time, first] = timed(parallel_function);
            auto [serial_time, second] = timed(serial_function);
            parallel_seconds.push_back(parallel_time);
            serial_seconds.push_back(serial_time);
            parallel_result = first;
            serial_result = second;
            every_result_equal = every_result_equal
                && same_numerical_result(first, second);
        }
    }
    const double serial_median = median(serial_seconds);
    const double parallel_median = median(parallel_seconds);

    std::cout << "benchmark=" << name << '\n'
              << "numerical_results_equal="
              << (every_result_equal ? "true" : "false")
              << '\n'
              << "price=" << serial_result.price << '\n'
              << "standard_error=" << serial_result.standard_error << '\n'
              << "threads_used=" << parallel_result.threads_used << '\n';
    print_times("serial_seconds", serial_seconds);
    print_times("parallel_seconds", parallel_seconds);
    std::cout << "serial_median_seconds=" << serial_median << '\n'
              << "parallel_median_seconds=" << parallel_median << '\n'
              << "median_speedup=" << serial_median / parallel_median << '\n';
    if (!every_result_equal) {
        throw std::runtime_error(
            std::string{name} + " serial and OpenMP results differ"
        );
    }
}

void run(const Settings& settings)
{
    if (!derivatives::openmp_available()) {
        throw std::runtime_error("benchmark requires an OpenMP-enabled build");
    }
    const auto option = reference_option();
    const derivatives::AsianOption asian{
        .underlying = option,
        .monitoring_steps = settings.asian_steps,
    };
    const derivatives::MonteCarloConfig european_serial{
        .paths = settings.european_paths,
        .seed = 20'260'731U,
    };
    auto european_parallel = european_serial;
    european_parallel.execution = derivatives::ExecutionPolicy::OpenMP;
    european_parallel.threads = settings.threads;

    const derivatives::MonteCarloConfig asian_serial{
        .paths = settings.asian_paths,
        .seed = 20'260'731U,
        .variance_reduction = derivatives::VarianceReduction::Antithetic,
    };
    auto asian_parallel = asian_serial;
    asian_parallel.execution = derivatives::ExecutionPolicy::OpenMP;
    asian_parallel.threads = settings.threads;

    std::cout << std::setprecision(9)
              << "clock=std::chrono::steady_clock\n"
              << "repetitions=" << settings.repetitions << '\n'
              << "requested_threads=" << settings.threads << '\n'
              << "european_paths=" << settings.european_paths << '\n'
              << "asian_paths=" << settings.asian_paths << '\n'
              << "asian_monitoring_steps=" << settings.asian_steps << '\n';

    benchmark_pair(
        "european_plain",
        settings.repetitions,
        [&] {
            return derivatives::price_european_monte_carlo(
                option,
                european_serial
            );
        },
        [&] {
            return derivatives::price_european_monte_carlo(
                option,
                european_parallel
            );
        }
    );
    benchmark_pair(
        "asian_antithetic",
        settings.repetitions,
        [&] { return derivatives::price_asian_monte_carlo(asian, asian_serial); },
        [&] {
            return derivatives::price_asian_monte_carlo(asian, asian_parallel);
        }
    );
}

}  // namespace

int main(int argc, char* argv[])
{
    try {
        run(parse_settings(argc, argv));
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "error: " << exception.what() << '\n';
        return 1;
    }
}
