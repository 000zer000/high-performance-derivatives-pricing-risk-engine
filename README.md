# High-Performance Derivatives Pricing & Risk Engine

Status: **v1.0.0 release candidate**. The complete scope below is implemented
and locally verified. Public CI and clean-clone verification are the remaining
release gates.

A correctness-first C++20 pricing engine that connects quantitative finance,
statistical validation, reproducible simulation, Python interoperability, and
measured shared-memory parallelism. All examples use synthetic parameters.

## Evidence-backed scope

- Black–Scholes prices for no-dividend European calls and puts;
- analytical delta, gamma, and vega, checked against independent values and
  central finite differences;
- Monte Carlo pricing for European, arithmetic-average Asian, and discretely
  monitored down-and-out options;
- standard errors and two-sided normal-approximation 95% confidence intervals;
- caller-controlled seeds and path-indexed deterministic random draws;
- antithetic variates for all Monte Carlo contracts;
- an analytically derived discounted-terminal-stock control variate for
  European options;
- fixed-order block reduction, giving identical serial and OpenMP results on a
  fixed binary/toolchain;
- optional OpenMP, optional pybind11 bindings, CLI, CMake, CTest, sanitizers,
  and Linux/macOS CI configuration;
- committed raw fixed-hardware timings, a serial profile extract, and a
  30-seed convergence dataset.

## Measured results, not promises

Fixed hardware: Apple M1, 8 cores, 8 GiB RAM, Apple Clang 15.0.0, Release
build, libomp 22.1.8, five timed repetitions after warm-up.

| Workload | Serial median | OpenMP median | Measured speedup | Equality |
|---|---:|---:|---:|---|
| 5,000,000-path European | 0.383474 s | 0.069511 s | 5.517x | exact |
| 300,000-path, 64-step Asian antithetic | 1.336124 s | 0.242718 s | 5.505x | exact |

The timings apply only to the recorded machine and commands. The Asian timings
showed visible run-to-run variability. See
[`benchmarks/results/2026-07-31-apple-m1.txt`](benchmarks/results/2026-07-31-apple-m1.txt)
for every repetition and limitation.

In the 30-seed European call study, plain-estimator RMSE fell from `0.348672`
at 2,000 paths to `0.010255` at 2,000,000 paths. At 2,000,000 paths, observed
RMSE was `0.007854` with antithetic sampling and `0.004564` with the control
variate. This is a finite experiment, not a general accuracy guarantee.

## Build and test the C++ project

Prerequisites:

- CMake 3.25 or newer;
- a C++20 compiler;
- OpenMP is optional.

```bash
git clone https://github.com/000zer000/high-performance-derivatives-pricing-risk-engine.git
cd high-performance-derivatives-pricing-risk-engine
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DDPR_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On Apple Silicon with Homebrew OpenMP:

```bash
brew install libomp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DDPR_WARNINGS_AS_ERRORS=ON \
  -DDPR_ENABLE_OPENMP=ON
```

OpenMP remains optional: a machine without it can build and test the serial
engine. Requesting OpenMP execution from such a build raises a clear error.

## Build and test the Python bindings

Install pybind11 for the Python interpreter CMake will use:

```bash
python3 -m pip install pybind11==3.0.4
cmake -S . -B build-python -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DDPR_WARNINGS_AS_ERRORS=ON \
  -DDPR_BUILD_PYTHON=ON \
  -Dpybind11_DIR="$(python3 -m pybind11 --cmakedir)"
cmake --build build-python --parallel
ctest --test-dir build-python --output-on-failure
```

The Python module is a thin binding over `pricing_core`; financial formulas
are not duplicated in Python.

## CLI examples

```bash
# Analytical price and Greeks
./build/pricing_cli analytical call \
  --spot 100 --strike 100 --rate 0.05 --volatility 0.20 --maturity 1
./build/pricing_cli greeks call \
  --spot 100 --strike 100 --rate 0.05 --volatility 0.20 --maturity 1

# European control variate on eight OpenMP threads
./build/pricing_cli monte-carlo european call \
  --spot 100 --strike 100 --rate 0.05 --volatility 0.20 --maturity 1 \
  --paths 2000000 --seed 20260731 \
  --variance-reduction control --execution openmp --threads 8

# Arithmetic Asian with antithetic pairs
./build/pricing_cli monte-carlo asian call \
  --spot 100 --strike 100 --rate 0.05 --volatility 0.20 --maturity 1 \
  --steps 64 --paths 300000 --seed 20260731 \
  --variance-reduction antithetic

# Discretely monitored down-and-out option with zero rebate
./build/pricing_cli monte-carlo down-and-out call \
  --spot 100 --strike 100 --rate 0.05 --volatility 0.20 --maturity 1 \
  --barrier 80 --steps 64 --paths 300000 --seed 20260731
```

Use `./build/pricing_cli --help` for the complete contract. Output is
machine-readable `key=value` data, including inputs, seed, path count,
effective sample count, thread count, estimator, price, standard error, and
interval bounds.

## Reproduce the evidence

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
  -DDPR_ENABLE_OPENMP=ON -DDPR_BUILD_BENCHMARKS=ON
cmake --build build-release --parallel
./build-release/pricing_benchmark 5000000 300000 64 5 8

python3 experiments/convergence.py \
  --cli build-release/pricing_cli \
  --paths 2000,20000,200000,2000000 --trials 30
```

## Explicit limitations

- Black–Scholes assumptions are constant rate/volatility, no dividends, and
  European exercise.
- Asian monitoring excludes the initial spot and uses equally spaced future
  observations.
- Barrier monitoring is discrete, includes the initial spot, uses zero rebate,
  and has monitoring bias relative to a continuously monitored barrier.
- Greeks are analytical delta/gamma/vega only; Monte Carlo Greeks, theta, and
  rho are not implemented.
- The 95% interval is a normal approximation for sampling error. It does not
  include model risk, parameter uncertainty, or barrier discretization error.
- Floating math can differ across compilers and standard libraries, so
  cross-platform bitwise identity is not claimed.
- This is a portfolio engine, not production trading or risk infrastructure.

Read the claim-by-claim status in [`docs/evidence_log.md`](docs/evidence_log.md),
formulas in [`docs/mathematics.md`](docs/mathematics.md), architecture in
[`docs/architecture.md`](docs/architecture.md), and reproducibility contract in
[`docs/reproducibility.md`](docs/reproducibility.md).

## Confidentiality

The repository uses mathematical models and synthetic parameters only. It
contains no employer data, source code, schema, identifier, screenshot,
credential, or confidential business information.

## License

MIT
