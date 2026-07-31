#include "derivatives/running_statistics.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace derivatives {

void RunningStatistics::add(double value)
{
    if (!std::isfinite(value)) {
        throw std::invalid_argument("running-statistics value must be finite");
    }

    ++count_;
    const double delta = value - mean_;
    mean_ += delta / static_cast<double>(count_);
    const double delta_after_mean_update = value - mean_;
    sum_squared_deviations_ += delta * delta_after_mean_update;
}

void RunningStatistics::merge(const RunningStatistics& other)
{
    if (other.count_ == 0U) {
        return;
    }
    if (count_ == 0U) {
        *this = other;
        return;
    }
    if (count_ > std::numeric_limits<std::uint64_t>::max() - other.count_) {
        throw std::overflow_error("running-statistics sample count overflow");
    }

    const std::uint64_t combined_count = count_ + other.count_;
    const double count = static_cast<double>(count_);
    const double other_count = static_cast<double>(other.count_);
    const double combined_count_as_double =
        static_cast<double>(combined_count);
    const double mean_difference = other.mean_ - mean_;

    sum_squared_deviations_ += other.sum_squared_deviations_
        + mean_difference * mean_difference * count * other_count
            / combined_count_as_double;
    mean_ += mean_difference * other_count / combined_count_as_double;
    count_ = combined_count;
}

std::uint64_t RunningStatistics::count() const noexcept
{
    return count_;
}

double RunningStatistics::mean() const
{
    if (count_ == 0U) {
        throw std::logic_error("mean requires at least one sample");
    }
    return mean_;
}

double RunningStatistics::sample_variance() const
{
    if (count_ < 2U) {
        throw std::logic_error("sample variance requires at least two samples");
    }
    return sum_squared_deviations_ / static_cast<double>(count_ - 1U);
}

double RunningStatistics::standard_error() const
{
    return std::sqrt(sample_variance() / static_cast<double>(count_));
}

}  // namespace derivatives
