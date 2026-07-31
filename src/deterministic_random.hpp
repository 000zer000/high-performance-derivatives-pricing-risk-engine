#pragma once

#include <cstdint>

namespace derivatives::detail {

/// Deterministic standard-normal draw addressed by seed, path, and time step.
double normal_draw(
    std::uint64_t seed,
    std::uint64_t path,
    std::uint64_t step
);

}  // namespace derivatives::detail
