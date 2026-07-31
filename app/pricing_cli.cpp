#include "derivatives/black_scholes.hpp"
#include "derivatives/greeks.hpp"
#include "derivatives/monte_carlo.hpp"

#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

enum class Model : std::uint8_t {
    Analytical,
    Greeks,
    MonteCarlo,
};

enum class Contract : std::uint8_t {
    European,
    Asian,
    DownAndOut,
};

struct ParsedArguments {
    Model model{Model::Analytical};
    Contract contract{Contract::European};
    derivatives::EuropeanOption option{};
    derivatives::MonteCarloConfig monte_carlo{};
    std::uint32_t monitoring_steps{};
    double barrier{};
};

void print_usage(std::ostream& output)
{
    output
        << "Usage:\n"
        << "  pricing_cli analytical <call|put> [market inputs]\n"
        << "  pricing_cli greeks <call|put> [market inputs]\n"
        << "  pricing_cli monte-carlo [european] <call|put> [inputs] "
           "--paths N --seed N\n"
        << "  pricing_cli monte-carlo asian <call|put> [inputs] "
           "--steps N --paths N --seed N\n"
        << "  pricing_cli monte-carlo down-and-out <call|put> [inputs] "
           "--barrier X --steps N --paths N --seed N\n\n"
        << "Required market inputs:\n"
        << "  --spot X --strike X --rate X --volatility X --maturity X\n\n"
        << "Monte Carlo options:\n"
        << "  --variance-reduction <none|antithetic|control>\n"
        << "  --execution <serial|openmp>\n"
        << "  --threads N  OpenMP team size; omit for runtime default\n";
}

Model parse_model(std::string_view text)
{
    if (text == "analytical") {
        return Model::Analytical;
    }
    if (text == "greeks") {
        return Model::Greeks;
    }
    if (text == "monte-carlo") {
        return Model::MonteCarlo;
    }
    throw std::invalid_argument(
        "model must be analytical, greeks, or monte-carlo"
    );
}

Contract parse_contract(std::string_view text)
{
    if (text == "european") {
        return Contract::European;
    }
    if (text == "asian") {
        return Contract::Asian;
    }
    if (text == "down-and-out") {
        return Contract::DownAndOut;
    }
    throw std::invalid_argument(
        "contract must be european, asian, or down-and-out"
    );
}

bool is_contract(std::string_view text)
{
    return text == "european" || text == "asian" || text == "down-and-out";
}

derivatives::OptionType parse_option_type(std::string_view text)
{
    if (text == "call") {
        return derivatives::OptionType::Call;
    }
    if (text == "put") {
        return derivatives::OptionType::Put;
    }
    throw std::invalid_argument("option type must be call or put");
}

derivatives::VarianceReduction parse_variance_reduction(std::string_view text)
{
    if (text == "none") {
        return derivatives::VarianceReduction::None;
    }
    if (text == "antithetic") {
        return derivatives::VarianceReduction::Antithetic;
    }
    if (text == "control") {
        return derivatives::VarianceReduction::ControlVariate;
    }
    throw std::invalid_argument(
        "variance reduction must be none, antithetic, or control"
    );
}

derivatives::ExecutionPolicy parse_execution(std::string_view text)
{
    if (text == "serial") {
        return derivatives::ExecutionPolicy::Serial;
    }
    if (text == "openmp") {
        return derivatives::ExecutionPolicy::OpenMP;
    }
    throw std::invalid_argument("execution must be serial or openmp");
}

double parse_double(std::string_view text, std::string_view argument_name)
{
    const std::string null_terminated_text{text};
    char* position{};
    errno = 0;
    const double value = std::strtod(null_terminated_text.c_str(), &position);
    if (position == null_terminated_text.c_str() || *position != '\0'
        || errno == ERANGE) {
        throw std::invalid_argument(
            std::string{argument_name} + " requires a valid number"
        );
    }
    return value;
}

std::uint64_t parse_unsigned_integer(
    std::string_view text,
    std::string_view argument_name
)
{
    std::uint64_t value{};
    const char* const begin = text.data();
    const char* const end = begin + text.size();
    const auto [position, error] = std::from_chars(begin, end, value);
    if (error != std::errc{} || position != end) {
        throw std::invalid_argument(
            std::string{argument_name} + " requires a non-negative integer"
        );
    }
    return value;
}

std::uint32_t parse_unsigned_32(
    std::string_view text,
    std::string_view argument_name
)
{
    const std::uint64_t value = parse_unsigned_integer(text, argument_name);
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(
            std::string{argument_name} + " exceeds the 32-bit limit"
        );
    }
    return static_cast<std::uint32_t>(value);
}

void require_once(bool already_set, std::string_view argument_name)
{
    if (already_set) {
        throw std::invalid_argument(
            std::string{argument_name} + " may be provided only once"
        );
    }
}

ParsedArguments parse_arguments(int argc, char* argv[])
{
    if (argc < 3) {
        throw std::invalid_argument("model and option type are required");
    }

    ParsedArguments parsed;
    parsed.model = parse_model(argv[1]);
    int first_flag = 3;
    if (parsed.model == Model::MonteCarlo && is_contract(argv[2])) {
        if (argc < 4) {
            throw std::invalid_argument("option type is required");
        }
        parsed.contract = parse_contract(argv[2]);
        parsed.option.type = parse_option_type(argv[3]);
        first_flag = 4;
    } else {
        parsed.option.type = parse_option_type(argv[2]);
    }

    bool has_spot = false;
    bool has_strike = false;
    bool has_rate = false;
    bool has_volatility = false;
    bool has_maturity = false;
    bool has_paths = false;
    bool has_seed = false;
    bool has_steps = false;
    bool has_barrier = false;
    bool has_variance_reduction = false;
    bool has_execution = false;
    bool has_threads = false;

    for (int index = first_flag; index < argc; index += 2) {
        const std::string_view argument{argv[index]};
        if (index + 1 >= argc) {
            throw std::invalid_argument(
                std::string{argument} + " requires a value"
            );
        }
        const std::string_view value{argv[index + 1]};
        if (argument == "--spot") {
            require_once(has_spot, argument);
            parsed.option.spot = parse_double(value, argument);
            has_spot = true;
        } else if (argument == "--strike") {
            require_once(has_strike, argument);
            parsed.option.strike = parse_double(value, argument);
            has_strike = true;
        } else if (argument == "--rate") {
            require_once(has_rate, argument);
            parsed.option.risk_free_rate = parse_double(value, argument);
            has_rate = true;
        } else if (argument == "--volatility") {
            require_once(has_volatility, argument);
            parsed.option.volatility = parse_double(value, argument);
            has_volatility = true;
        } else if (argument == "--maturity") {
            require_once(has_maturity, argument);
            parsed.option.time_to_maturity = parse_double(value, argument);
            has_maturity = true;
        } else if (argument == "--paths") {
            require_once(has_paths, argument);
            parsed.monte_carlo.paths = parse_unsigned_integer(value, argument);
            has_paths = true;
        } else if (argument == "--seed") {
            require_once(has_seed, argument);
            parsed.monte_carlo.seed = parse_unsigned_integer(value, argument);
            has_seed = true;
        } else if (argument == "--steps") {
            require_once(has_steps, argument);
            parsed.monitoring_steps = parse_unsigned_32(value, argument);
            has_steps = true;
        } else if (argument == "--barrier") {
            require_once(has_barrier, argument);
            parsed.barrier = parse_double(value, argument);
            has_barrier = true;
        } else if (argument == "--variance-reduction") {
            require_once(has_variance_reduction, argument);
            parsed.monte_carlo.variance_reduction =
                parse_variance_reduction(value);
            has_variance_reduction = true;
        } else if (argument == "--execution") {
            require_once(has_execution, argument);
            parsed.monte_carlo.execution = parse_execution(value);
            has_execution = true;
        } else if (argument == "--threads") {
            require_once(has_threads, argument);
            parsed.monte_carlo.threads = parse_unsigned_32(value, argument);
            has_threads = true;
        } else {
            throw std::invalid_argument(
                std::string{"unknown argument: "} + std::string{argument}
            );
        }
    }

    if (!has_spot || !has_strike || !has_rate || !has_volatility
        || !has_maturity) {
        throw std::invalid_argument("all five market inputs are required");
    }
    const bool has_monte_carlo_argument = has_paths || has_seed || has_steps
        || has_barrier || has_variance_reduction || has_execution || has_threads;
    if (parsed.model != Model::MonteCarlo && has_monte_carlo_argument) {
        throw std::invalid_argument(
            "analytical and greeks commands do not accept Monte Carlo inputs"
        );
    }
    if (parsed.model == Model::MonteCarlo && (!has_paths || !has_seed)) {
        throw std::invalid_argument(
            "monte-carlo requires both --paths and --seed"
        );
    }
    if (parsed.contract == Contract::European && (has_steps || has_barrier)) {
        throw std::invalid_argument(
            "European Monte Carlo does not accept --steps or --barrier"
        );
    }
    if (parsed.contract == Contract::Asian && (!has_steps || has_barrier)) {
        throw std::invalid_argument(
            "Asian Monte Carlo requires --steps and does not accept --barrier"
        );
    }
    if (parsed.contract == Contract::DownAndOut
        && (!has_steps || !has_barrier)) {
        throw std::invalid_argument(
            "down-and-out Monte Carlo requires --steps and --barrier"
        );
    }
    return parsed;
}

std::string_view option_type_name(derivatives::OptionType type)
{
    return type == derivatives::OptionType::Call ? "call" : "put";
}

std::string_view contract_name(Contract contract)
{
    if (contract == Contract::Asian) {
        return "arithmetic_asian";
    }
    if (contract == Contract::DownAndOut) {
        return "down_and_out";
    }
    return "european";
}

std::string_view variance_reduction_name(
    derivatives::VarianceReduction method
)
{
    if (method == derivatives::VarianceReduction::Antithetic) {
        return "antithetic";
    }
    if (method == derivatives::VarianceReduction::ControlVariate) {
        return "control";
    }
    return "none";
}

std::string_view execution_name(derivatives::ExecutionPolicy execution)
{
    return execution == derivatives::ExecutionPolicy::OpenMP
        ? "openmp"
        : "serial";
}

void print_inputs(const ParsedArguments& arguments)
{
    std::cout << std::setprecision(17)
              << "option_type=" << option_type_name(arguments.option.type)
              << '\n'
              << "spot=" << arguments.option.spot << '\n'
              << "strike=" << arguments.option.strike << '\n'
              << "risk_free_rate=" << arguments.option.risk_free_rate << '\n'
              << "volatility=" << arguments.option.volatility << '\n'
              << "time_to_maturity="
              << arguments.option.time_to_maturity << '\n';
}

void print_monte_carlo_result(
    const ParsedArguments& arguments,
    const derivatives::MonteCarloResult& result
)
{
    std::cout << "model=monte_carlo\n"
              << "contract=" << contract_name(arguments.contract) << '\n';
    print_inputs(arguments);
    if (arguments.contract != Contract::European) {
        std::cout << "monitoring_steps=" << arguments.monitoring_steps << '\n';
    }
    if (arguments.contract == Contract::DownAndOut) {
        std::cout << std::setprecision(17)
                  << "barrier=" << arguments.barrier << '\n';
    }
    std::cout << "variance_reduction="
              << variance_reduction_name(
                     arguments.monte_carlo.variance_reduction
                 )
              << '\n'
              << "execution=" << execution_name(arguments.monte_carlo.execution)
              << '\n'
              << std::setprecision(17)
              << "paths=" << result.paths << '\n'
              << "effective_samples=" << result.effective_samples << '\n'
              << "seed=" << result.seed << '\n'
              << "threads_used=" << result.threads_used << '\n'
              << "control_variate_beta=" << result.control_variate_beta << '\n'
              << "price=" << result.price << '\n'
              << "standard_error=" << result.standard_error << '\n'
              << "ci95_lower=" << result.confidence_interval_lower << '\n'
              << "ci95_upper=" << result.confidence_interval_upper << '\n';
}

void run(const ParsedArguments& arguments)
{
    if (arguments.model == Model::Analytical) {
        std::cout << "model=black_scholes\n";
        print_inputs(arguments);
        std::cout << "price=" << std::setprecision(17)
                  << derivatives::black_scholes_price(arguments.option) << '\n';
        return;
    }
    if (arguments.model == Model::Greeks) {
        const auto greeks = derivatives::black_scholes_greeks(arguments.option);
        std::cout << "model=black_scholes_greeks\n";
        print_inputs(arguments);
        std::cout << std::setprecision(17)
                  << "delta=" << greeks.delta << '\n'
                  << "gamma=" << greeks.gamma << '\n'
                  << "vega=" << greeks.vega << '\n';
        return;
    }

    derivatives::MonteCarloResult result;
    if (arguments.contract == Contract::European) {
        result = derivatives::price_european_monte_carlo(
            arguments.option,
            arguments.monte_carlo
        );
    } else if (arguments.contract == Contract::Asian) {
        result = derivatives::price_asian_monte_carlo(
            {
                .underlying = arguments.option,
                .monitoring_steps = arguments.monitoring_steps,
            },
            arguments.monte_carlo
        );
    } else {
        result = derivatives::price_down_and_out_monte_carlo(
            {
                .underlying = arguments.option,
                .barrier = arguments.barrier,
                .monitoring_steps = arguments.monitoring_steps,
            },
            arguments.monte_carlo
        );
    }
    print_monte_carlo_result(arguments, result);
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc == 2
        && (std::string_view{argv[1]} == "--help"
            || std::string_view{argv[1]} == "-h")) {
        print_usage(std::cout);
        return 0;
    }
    try {
        run(parse_arguments(argc, argv));
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "error: " << exception.what() << '\n'
                  << "Run pricing_cli --help for usage.\n";
        return 1;
    }
}
