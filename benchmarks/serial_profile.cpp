#include "derivatives/monte_carlo.hpp"

#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

std::uint64_t parse_positive(std::string_view text, std::string_view name)
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

}  // namespace

int main(int argc, char* argv[])
{
    try {
        if (argc != 3) {
            throw std::invalid_argument("expected: paths monitoring_steps");
        }
        const std::uint64_t paths = parse_positive(argv[1], "paths");
        const std::uint64_t steps_value =
            parse_positive(argv[2], "monitoring_steps");
        if (steps_value > static_cast<std::uint64_t>(UINT32_MAX)) {
            throw std::invalid_argument("monitoring_steps is too large");
        }
        const auto steps = static_cast<std::uint32_t>(steps_value);
        const derivatives::AsianOption option{
            .underlying = {
                .spot = 100.0,
                .strike = 100.0,
                .risk_free_rate = 0.05,
                .volatility = 0.20,
                .time_to_maturity = 1.0,
                .type = derivatives::OptionType::Call,
            },
            .monitoring_steps = steps,
        };
        std::cout << "profile=serial_asian_antithetic\n"
                  << "paths=" << paths << '\n'
                  << "monitoring_steps=" << steps << '\n'
                  << std::flush;
        const auto result = derivatives::price_asian_monte_carlo(
            option,
            {
                .paths = paths,
                .seed = 20'260'731U,
                .variance_reduction =
                    derivatives::VarianceReduction::Antithetic,
            }
        );
        std::cout << std::setprecision(17)
                  << "price=" << result.price << '\n'
                  << "standard_error=" << result.standard_error << '\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "error: " << exception.what() << '\n';
        return 1;
    }
}
