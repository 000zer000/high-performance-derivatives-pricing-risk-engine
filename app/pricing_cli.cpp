#include "derivatives/black_scholes.hpp"
#include "derivatives/monte_carlo.hpp"

#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

enum class Model : std::uint8_t {
    Analytical,
    MonteCarlo,
};

struct ParsedArguments {
    Model model{Model::Analytical};
    derivatives::EuropeanOption option{};
    derivatives::MonteCarloConfig monte_carlo{};
};

void print_usage(std::ostream& output)
{
    output
        << "Usage:\n"
        << "  pricing_cli analytical <call|put> [market inputs]\n"
        << "  pricing_cli monte-carlo <call|put> [market inputs] "
           "--paths <integer> --seed <integer>\n\n"
        << "Required market inputs:\n"
        << "  --spot <number>       Current underlying price (> 0)\n"
        << "  --strike <number>     Strike price (> 0)\n"
        << "  --rate <number>       Continuously compounded risk-free rate\n"
        << "  --volatility <number> Annual volatility (>= 0)\n"
        << "  --maturity <number>   Time to maturity in years (>= 0)\n";
}

Model parse_model(std::string_view text)
{
    if (text == "analytical") {
        return Model::Analytical;
    }
    if (text == "monte-carlo") {
        return Model::MonteCarlo;
    }
    throw std::invalid_argument("model must be analytical or monte-carlo");
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
    parsed.option.type = parse_option_type(argv[2]);

    bool has_spot = false;
    bool has_strike = false;
    bool has_rate = false;
    bool has_volatility = false;
    bool has_maturity = false;
    bool has_paths = false;
    bool has_seed = false;

    for (int index = 3; index < argc; index += 2) {
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
    if (parsed.model == Model::MonteCarlo && (!has_paths || !has_seed)) {
        throw std::invalid_argument(
            "monte-carlo requires both --paths and --seed"
        );
    }
    if (parsed.model == Model::Analytical && (has_paths || has_seed)) {
        throw std::invalid_argument(
            "analytical does not accept --paths or --seed"
        );
    }

    return parsed;
}

std::string_view option_type_name(derivatives::OptionType type)
{
    return type == derivatives::OptionType::Call ? "call" : "put";
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

void run(const ParsedArguments& arguments)
{
    if (arguments.model == Model::Analytical) {
        const double price =
            derivatives::black_scholes_price(arguments.option);
        std::cout << "model=black_scholes\n";
        print_inputs(arguments);
        std::cout << "price=" << std::setprecision(17) << price << '\n';
        return;
    }

    const auto result = derivatives::price_european_monte_carlo(
        arguments.option,
        arguments.monte_carlo
    );
    std::cout << "model=monte_carlo\n";
    print_inputs(arguments);
    std::cout << std::setprecision(17) << "paths=" << result.paths << '\n'
              << "seed=" << result.seed << '\n'
              << "price=" << result.price << '\n'
              << "standard_error=" << result.standard_error << '\n'
              << "ci95_lower=" << result.confidence_interval_lower << '\n'
              << "ci95_upper=" << result.confidence_interval_upper << '\n';
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
        std::cerr << "error: " << exception.what() << '\n';
        std::cerr << "Run pricing_cli --help for usage.\n";
        return 1;
    }
}
