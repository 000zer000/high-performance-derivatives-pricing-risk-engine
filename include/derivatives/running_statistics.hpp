#pragma once

#include <cstdint>

namespace derivatives {

/// One-pass sample statistics using Welford updates and pairwise merging.
class RunningStatistics {
public:
    /// Adds one finite sample; throws std::invalid_argument otherwise.
    void add(double value);
    /// Combines another accumulator without replaying its samples.
    void merge(const RunningStatistics& other);

    [[nodiscard]] std::uint64_t count() const noexcept;
    /// Requires at least one sample.
    [[nodiscard]] double mean() const;
    /// Uses denominator n - 1 and requires at least two samples.
    [[nodiscard]] double sample_variance() const;
    /// Returns sample standard deviation divided by sqrt(n).
    [[nodiscard]] double standard_error() const;

private:
    std::uint64_t count_{};
    double mean_{};
    double sum_squared_deviations_{};
};

}  // namespace derivatives
