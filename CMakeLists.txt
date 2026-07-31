cmake_minimum_required(VERSION 3.25)

project(
    DerivativesPricingRiskEngine
    VERSION 0.1.0
    DESCRIPTION "Correctness-first derivatives pricing and risk engine"
    LANGUAGES CXX
)

option(DPR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(dpr_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
        if(DPR_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(
            ${target_name}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wshadow
        )
        if(DPR_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

add_library(
    pricing_core
    STATIC
        src/black_scholes.cpp
        src/monte_carlo.cpp
        src/option.cpp
        src/running_statistics.cpp
)

target_compile_features(pricing_core PUBLIC cxx_std_20)

target_include_directories(
    pricing_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

dpr_enable_warnings(pricing_core)

add_executable(pricing_cli app/pricing_cli.cpp)
target_compile_features(pricing_cli PRIVATE cxx_std_20)
target_link_libraries(pricing_cli PRIVATE pricing_core)
dpr_enable_warnings(pricing_cli)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
