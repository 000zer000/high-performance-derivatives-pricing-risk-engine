cmake_minimum_required(VERSION 3.25)

project(
    DerivativesPricingRiskEngine
    VERSION 1.0.0
    DESCRIPTION "Correctness-first derivatives pricing and risk engine"
    LANGUAGES CXX
)

option(DPR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(DPR_ENABLE_OPENMP "Build the optional OpenMP execution policy" ON)
option(DPR_BUILD_PYTHON "Build the optional pybind11 module" OFF)
option(DPR_BUILD_BENCHMARKS "Build benchmark executables" OFF)

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
        src/deterministic_random.cpp
        src/greeks.cpp
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
set_target_properties(pricing_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

dpr_enable_warnings(pricing_core)

set(DPR_OPENMP_AVAILABLE OFF)
if(DPR_ENABLE_OPENMP)
    if(APPLE)
        find_program(DPR_HOMEBREW_EXECUTABLE brew)
        if(DPR_HOMEBREW_EXECUTABLE)
            execute_process(
                COMMAND ${DPR_HOMEBREW_EXECUTABLE} --prefix libomp
                OUTPUT_VARIABLE DPR_LIBOMP_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        endif()
    endif()
    if(APPLE AND EXISTS "${DPR_LIBOMP_PREFIX}/lib/libomp.dylib")
        set(OpenMP_CXX_FLAGS
            "-Xpreprocessor -fopenmp -I${DPR_LIBOMP_PREFIX}/include"
        )
        set(OpenMP_CXX_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY "${DPR_LIBOMP_PREFIX}/lib/libomp.dylib")
    endif()
    find_package(OpenMP QUIET COMPONENTS CXX)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(pricing_core PUBLIC OpenMP::OpenMP_CXX)
        target_compile_definitions(pricing_core PRIVATE DPR_HAS_OPENMP=1)
        set(DPR_OPENMP_AVAILABLE ON)
        message(STATUS "OpenMP execution policy enabled")
    else()
        message(STATUS "OpenMP not found; serial execution remains available")
    endif()
endif()

add_executable(pricing_cli app/pricing_cli.cpp)
target_compile_features(pricing_cli PRIVATE cxx_std_20)
target_link_libraries(pricing_cli PRIVATE pricing_core)
dpr_enable_warnings(pricing_cli)

if(DPR_BUILD_PYTHON)
    find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(derivatives_engine python/bindings.cpp)
    target_link_libraries(derivatives_engine PRIVATE pricing_core)
    target_compile_features(derivatives_engine PRIVATE cxx_std_20)
    dpr_enable_warnings(derivatives_engine)
    if(BUILD_TESTING)
        add_test(
            NAME python.bindings
            COMMAND
                ${Python_EXECUTABLE}
                ${PROJECT_SOURCE_DIR}/python/tests/test_bindings.py
        )
        set_tests_properties(
            python.bindings
            PROPERTIES
                ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:derivatives_engine>"
        )
    endif()
endif()

if(DPR_BUILD_BENCHMARKS)
    add_executable(pricing_benchmark benchmarks/pricing_benchmark.cpp)
    target_link_libraries(pricing_benchmark PRIVATE pricing_core)
    target_compile_features(pricing_benchmark PRIVATE cxx_std_20)
    dpr_enable_warnings(pricing_benchmark)

    add_executable(serial_profile benchmarks/serial_profile.cpp)
    target_link_libraries(serial_profile PRIVATE pricing_core)
    target_compile_features(serial_profile PRIVATE cxx_std_20)
    dpr_enable_warnings(serial_profile)
endif()

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
