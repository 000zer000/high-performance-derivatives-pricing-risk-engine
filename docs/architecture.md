# Architecture

## v0.1.0 component boundaries

```text
pricing_cli ───────┐
CTest programs ───┼──> pricing_core ───> input validation
                  │                   ├─> Black–Scholes pricing
Future pybind11 ──┤                   ├─> serial Monte Carlo
Future benchmarks ┘                   └─> running statistics
```

`pricing_core` is a static C++20 library. It owns all financial and numerical
logic. The CLI parses and prints values but contains no pricing formula.

## Types and responsibilities

### `EuropeanOption`

A value type containing spot, strike, continuously compounded risk-free rate,
annual volatility, time to maturity in years, and call/put type.

`validate` rejects non-finite values, non-positive spot/strike, negative
volatility, negative maturity, and invalid option types. Negative finite rates
are allowed.

### `black_scholes_price`

Prices no-dividend European calls and puts. It handles expiry and zero
volatility explicitly before evaluating the non-degenerate closed form.

### `RunningStatistics`

Uses Welford’s update for one-pass mean and sample variance. Its merge formula
combines two independent accumulators without retaining individual samples.
That merge operation is implemented and tested now because later deterministic
parallel blocks will depend on it.

### `price_european_monte_carlo`

Validates inputs, owns one serial `std::mt19937_64` stream, simulates terminal
prices under risk-neutral geometric Brownian motion, discounts each payoff,
and returns the sample mean, standard error, 95% interval, path count, and seed.

### `pricing_cli`

Accepts explicit analytical or Monte Carlo commands. It reports every input
needed to reproduce a result. Parsing failures and domain failures return a
nonzero exit status with an error message.

## CMake targets

- `pricing_core`: reusable numerical library;
- `pricing_cli`: command-line adapter linked to `pricing_core`;
- `test_build_smoke`: public-type compile/link smoke test;
- `test_option_validation`: named validation cases;
- `test_running_statistics`: one-pass and merge cases;
- `test_black_scholes`: analytical reference and boundary cases;
- `test_monte_carlo`: seed, uncertainty, boundary, and error-screen cases.

CTest also invokes `pricing_cli` directly for six command-line behavior cases.

## Deferred architecture

- Greeks and their finite-difference validation;
- antithetic and control-variate estimators;
- Asian and barrier option types;
- pybind11 and Python packaging;
- profiling and benchmark targets;
- deterministic block scheduling and OpenMP.

None of these future components is implied by the v0.1.0 API.
