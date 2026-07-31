#include "deterministic_random.hpp"

#include <cmath>
#include <numbers>

namespace derivatives::detail {

namespace {

constexpr std::uint64_t golden_ratio = 0x9e3779b97f4a7c15ULL;

std::uint64_t splitmix64(std::uint64_t value)
{
    value += golden_ratio;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

double open_unit_interval(std::uint64_t bits)
{
    constexpr double two_to_53 = 9'007'199'254'740'992.0;
    const std::uint64_t mantissa = bits >> 11U;
    return (static_cast<double>(mantissa) + 0.5) / two_to_53;
}

}  // namespace

double normal_draw(
    std::uint64_t seed,
    std::uint64_t path,
    std::uint64_t step
)
{
    const std::uint64_t key = splitmix64(seed)
        ^ splitmix64(path + golden_ratio)
        ^ splitmix64(step + 2U * golden_ratio);
    const double first_uniform = open_unit_interval(splitmix64(key));
    const double second_uniform =
        open_unit_interval(splitmix64(key + golden_ratio));
    return std::sqrt(-2.0 * std::log(first_uniform))
        * std::cos(2.0 * std::numbers::pi_v<double> * second_uniform);
}

}  // namespace derivatives::detail
