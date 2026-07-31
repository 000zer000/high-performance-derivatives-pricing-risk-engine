add_executable(test_build_smoke test_build_smoke.cpp)
target_compile_features(test_build_smoke PRIVATE cxx_std_20)
target_link_libraries(test_build_smoke PRIVATE pricing_core)
dpr_enable_warnings(test_build_smoke)

add_test(NAME scaffold.build_smoke COMMAND test_build_smoke)

add_executable(test_option_validation test_option_validation.cpp)
target_compile_features(test_option_validation PRIVATE cxx_std_20)
target_link_libraries(test_option_validation PRIVATE pricing_core)
dpr_enable_warnings(test_option_validation)

foreach(
    test_case
    IN ITEMS
        spot_non_positive
        strike_non_positive
        negative_volatility
        negative_maturity
        non_finite_inputs
        negative_rate_is_valid
        invalid_option_type
)
    add_test(
        NAME option_validation.${test_case}
        COMMAND test_option_validation ${test_case}
    )
endforeach()

add_executable(test_black_scholes test_black_scholes.cpp)
target_compile_features(test_black_scholes PRIVATE cxx_std_20)
target_link_libraries(test_black_scholes PRIVATE pricing_core)
dpr_enable_warnings(test_black_scholes)

foreach(
    test_case
    IN ITEMS
        reference_call
        reference_put
        put_call_parity
        zero_maturity_call
        zero_maturity_put
        zero_volatility
        negative_rate_is_supported
        call_nonnegative
        put_nonnegative
)
    add_test(
        NAME black_scholes.${test_case}
        COMMAND test_black_scholes ${test_case}
    )
endforeach()

add_executable(test_running_statistics test_running_statistics.cpp)
target_compile_features(test_running_statistics PRIVATE cxx_std_20)
target_link_libraries(test_running_statistics PRIVATE pricing_core)
dpr_enable_warnings(test_running_statistics)

foreach(
    test_case
    IN ITEMS
        empty_state
        known_sample
        merge_matches_single_pass
        requires_two_samples_for_standard_error
)
    add_test(
        NAME running_statistics.${test_case}
        COMMAND test_running_statistics ${test_case}
    )
endforeach()
