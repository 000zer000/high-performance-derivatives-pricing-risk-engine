# High-Performance Derivatives Pricing & Risk Engine

Status: **v0.1.0 correctness-first release candidate**. The analytical and
serial Monte Carlo European pricing scope is implemented and verified locally.
CI and public clean-clone verification for this implementation are pending.

The words “high-performance” describe the project roadmap, not a measured
performance claim. Python bindings, Greeks, variance reduction, path-dependent
options, profiling, OpenMP, and benchmarks remain explicitly deferred.

## Implemented v0.1.0 scope

- C++20 static pricing library built with CMake;
- Black–Scholes European call and put prices without dividends;
- serial terminal-price Monte Carlo using `std::mt19937_64`;
- caller-supplied seed and recorded path count;
- sample standard error and two-sided 95% confidence interval;
- tested zero-maturity and zero-volatility limits;
- finite-value and domain validation with explicit exceptions;
- Welford running mean/sample variance and merge operation;
- analytical and Monte Carlo command-line interface;
- named CTest cases and macOS/Linux GitHub Actions configuration.

## Build and test

Prerequisites:

- CMake 3.25 or newer;
- a C++20 compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DDPR_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On 31 July 2026, 36/36 CTest cases passed locally in both Debug and
Release with Apple Clang 15.0.0 and warnings treated as errors. This is not a
coverage percentage and does not establish production readiness.

## CLI examples

Analytical call:

```bash
./build/pricing_cli analytical call \
  --spot 100 --strike 100 --rate 0.05 \
  --volatility 0.20 --maturity 1
```

Serial Monte Carlo call:

```bash
./build/pricing_cli monte-carlo call \
  --spot 100 --strike 100 --rate 0.05 \
  --volatility 0.20 --maturity 1 \
  --paths 250000 --seed 20260731
```

Use `./build/pricing_cli --help` for the complete argument contract. Output is
printed as `key=value` lines and contains the model inputs and result metadata.

## Recorded numerical check

Frozen case: spot 100, strike 100, continuously compounded rate 0.05,
volatility 0.20, maturity 1 year, 250,000 paths, seed 20,260,731.

| Type | Analytical | Monte Carlo | Absolute error | Reported SE | Error / SE |
|---|---:|---:|---:|---:|---:|
| Call | 10.4505835722 | 10.4059583396 | 0.0446252326 | 0.0293098573 | 1.5225 |
| Put | 5.5735260223 | 5.5710527462 | 0.0024732761 | 0.0173067440 | 0.1429 |

The call interval was `[10.3485110192, 10.4634056599]`; the put interval was
`[5.5371315281, 5.6049739643]`. These are actual fixed-seed observations, not
promised accuracy. A multi-seed convergence study is required before making a
general convergence or error claim.

## Reproducibility boundary

The same Release Monte Carlo call command produced byte-for-byte identical
output when repeated on the audited toolchain. The C++ standard does not
require `std::normal_distribution` to generate identical sequences across
different standard-library implementations, so cross-platform bitwise
reproducibility is not claimed.

See:

- [`docs/mathematics.md`](docs/mathematics.md) for formulas and assumptions;
- [`docs/architecture.md`](docs/architecture.md) for component ownership;
- [`docs/reproducibility.md`](docs/reproducibility.md) for the exact boundary;
- [`docs/evidence_log.md`](docs/evidence_log.md) for claim status and evidence.

## Confidentiality

The repository uses mathematical models and synthetic parameters only. It
contains no employer data, code, schema, credential, screenshot, or
confidential business logic.

## License

MIT
