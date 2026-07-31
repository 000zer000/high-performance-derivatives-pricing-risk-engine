# Architecture

## Component boundaries

```text
pricing_cli ─────────┐
pybind11 module ─────┼──> pricing_core ───> contract validation
CTest programs ──────┤                   ├─> Black-Scholes price and Greeks
benchmark drivers ───┤                   ├─> deterministic random draws
convergence script ──┘                   ├─> Monte Carlo contract payoffs
                                        ├─> fixed block execution
                                        └─> running statistics
```

`pricing_core` is the only owner of financial and numerical logic. The CLI and
Python module translate inputs and expose results; neither duplicates formulas.
Benchmarks call the public core API used by normal applications.

## Public value types

- `EuropeanOption`: market inputs and call/put type;
- `AsianOption`: a European-style input set plus equally spaced monitoring
  count;
- `DownAndOutOption`: a European-style input set, positive barrier, and
  monitoring count;
- `MonteCarloConfig`: paths, seed, variance reduction, execution policy, and
  optional thread request;
- `MonteCarloResult`: price, standard error, interval, seed, path count,
  effective samples, actual threads, and control beta;
- `Greeks`: delta, gamma, and vega.

All public pricing functions validate their own inputs. Invalid domains use
standard exceptions, which pybind11 maps to Python exceptions.

## Monte Carlo execution

Each random draw is addressed by `(seed, path, step)`. A fixed block contains
2,048 statistically independent observations. Blocks are evaluated either in
serial or by an OpenMP `schedule(static)` loop. Every block preserves path
order, and completed block accumulators are merged in ascending block order.

This design has two useful consequences:

1. threads do not share a mutable random-number engine;
2. serial and OpenMP policies produce identical result fields on the same
   binary and math library, even when thread count changes.

Exceptions are captured per block and rethrown after the parallel region;
exceptions never escape directly through an OpenMP boundary.

## Variance reduction

Plain sampling treats every path payoff as one independent observation.
Antithetic sampling evaluates `Z` and `-Z` as a pair and treats their average
as one independent observation; therefore `effective_samples = paths / 2`.

The control variate is limited to European contracts because its beta is
derived analytically for the discounted terminal stock. Unsupported
path-dependent control requests are rejected rather than silently switching
estimators.

## Build options

- `DPR_WARNINGS_AS_ERRORS`: promote project warnings to build failures;
- `DPR_ENABLE_OPENMP`: detect and link OpenMP when available;
- `DPR_BUILD_PYTHON`: require pybind11 and build `derivatives_engine`;
- `DPR_BUILD_BENCHMARKS`: build timing and profiling drivers;
- standard `BUILD_TESTING`: register CTest cases.

The default build requires only CMake and a C++20 compiler. OpenMP, Python, and
benchmark targets are opt-in or gracefully optional.
