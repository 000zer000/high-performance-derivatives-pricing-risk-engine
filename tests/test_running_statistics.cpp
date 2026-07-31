#include "derivatives/running_statistics.hpp"

#include "test_support.hpp"

#include <cmath>
#include <stdexcept>
#include <string_view>

namespace {

bool test_empty_state()
{
    derivatives::RunningStatistics statistics;
    bool passed = expect_true(statistics.count() == 0U, "empty count is zero");
    passed = expect_throws_containing<std::logic_error>(
                 [&statistics] { static_cast<void>(statistics.mean()); },
                 "one sample",
                 "empty mean is unavailable"
             )
        && passed;
    passed = expect_throws_containing<std::logic_error>(
                 [&statistics] {
                     static_cast<void>(statistics.sample_variance());
                 },
                 "two samples",
                 "empty variance is unavailable"
             )
        && passed;
    return passed;
}

bool test_known_sample()
{
    derivatives::RunningStatistics statistics;
    for (const double value : {1.0, 2.0, 3.0, 4.0, 5.0}) {
        statistics.add(value);
    }

    bool passed = expect_true(statistics.count() == 5U, "sample count is five");
    passed = expect_near(statistics.mean(), 3.0, 1e-15, "known mean") && passed;
    passed = expect_near(
                 statistics.sample_variance(),
                 2.5,
                 1e-15,
                 "known sample variance"
             )
        && passed;
    passed = expect_near(
                 statistics.standard_error(),
                 std::sqrt(0.5),
                 1e-15,
                 "known standard error"
             )
        && passed;
    return passed;
}

bool test_merge_matches_single_pass()
{
    derivatives::RunningStatistics first_block;
    first_block.add(1.0);
    first_block.add(2.0);

    derivatives::RunningStatistics second_block;
    second_block.add(3.0);
    second_block.add(4.0);
    second_block.add(5.0);

    derivatives::RunningStatistics single_pass;
    for (const double value : {1.0, 2.0, 3.0, 4.0, 5.0}) {
        single_pass.add(value);
    }

    first_block.merge(second_block);
    bool passed = expect_true(first_block.count() == single_pass.count(), "merged count");
    passed = expect_near(first_block.mean(), single_pass.mean(), 1e-15, "merged mean")
        && passed;
    passed = expect_near(
                 first_block.sample_variance(),
                 single_pass.sample_variance(),
                 1e-15,
                 "merged sample variance"
             )
        && passed;
    return passed;
}

bool test_requires_two_samples_for_standard_error()
{
    derivatives::RunningStatistics statistics;
    statistics.add(42.0);
    return expect_throws_containing<std::logic_error>(
        [&statistics] { static_cast<void>(statistics.standard_error()); },
        "two samples",
        "one sample cannot produce a standard error"
    );
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 2;
    }

    const std::string_view test_name{argv[1]};
    if (test_name == "empty_state") {
        return test_empty_state() ? 0 : 1;
    }
    if (test_name == "known_sample") {
        return test_known_sample() ? 0 : 1;
    }
    if (test_name == "merge_matches_single_pass") {
        return test_merge_matches_single_pass() ? 0 : 1;
    }
    if (test_name == "requires_two_samples_for_standard_error") {
        return test_requires_two_samples_for_standard_error() ? 0 : 1;
    }
    return 2;
}
