#pragma once

#include <cmath>
#include <exception>
#include <iostream>
#include <string_view>
#include <utility>

inline bool expect_true(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

inline bool expect_near(
    double actual,
    double expected,
    double absolute_tolerance,
    std::string_view message
)
{
    const bool passed = std::abs(actual - expected) <= absolute_tolerance;
    if (!passed) {
        std::cerr << "FAILED: " << message << "; expected " << expected
                  << " +/- " << absolute_tolerance << ", received " << actual
                  << '\n';
    }
    return passed;
}

template <typename ExpectedException, typename Function>
bool expect_throws_containing(
    Function&& function,
    std::string_view expected_message_fragment,
    std::string_view message
)
{
    try {
        std::forward<Function>(function)();
    } catch (const ExpectedException& exception) {
        const std::string_view actual_message{exception.what()};
        const bool passed =
            actual_message.find(expected_message_fragment) != std::string_view::npos;
        if (!passed) {
            std::cerr << "FAILED: " << message << "; exception message '"
                      << actual_message << "' did not contain '"
                      << expected_message_fragment << "'\n";
        }
        return passed;
    } catch (const std::exception& exception) {
        std::cerr << "FAILED: " << message
                  << "; received a different standard exception: "
                  << exception.what() << '\n';
        return false;
    } catch (...) {
        std::cerr << "FAILED: " << message
                  << "; received a non-standard exception\n";
        return false;
    }

    std::cerr << "FAILED: " << message << "; no exception was thrown\n";
    return false;
}
