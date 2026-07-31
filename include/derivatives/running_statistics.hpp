#pragma once

#include <cstdint>

namespace derivatives {

class RunningStatistics {
public:
    void add(double value);
    void merge(const RunningStatistics& other);

    [[nodiscard]] std::uint64_t count() const noexcept;
    [[nodiscard]] double mean() const;
    [[nodiscard]] double sample_variance() const;
    [[nodiscard]] double standard_error() const;

private:
    std::uint64_t count_{};
    double mean_{};
    double sum_squared_deviations_{};
};

}  // namespace derivatives
