#pragma once

#include <iostream>
#include <string_view>

inline bool expect_true(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}
