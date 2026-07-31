# Architecture

## Current state

The repository is a build scaffold. Pricing and numerical algorithms are not
implemented.

## Intended component boundaries

```text
CLI ───────────────┐
Future pybind11 ───┼──> pricing_core ───> analytical pricing
Tests ─────────────┤                   ├─> serial Monte Carlo
Future benchmarks ─┘                   ├─> validation
                                        └─> running statistics
```

`pricing_core` will own financial and numerical logic. The CLI, future Python
binding, tests, and benchmarks will call that core rather than duplicate it.

## Current CMake targets

- `pricing_core`: static library containing implementation placeholders;
- `pricing_cli`: executable reporting scaffold status;
- `test_build_smoke`: compile/link smoke test registered with CTest.

## Deferred components

- analytical and Monte Carlo pricing implementations;
- Greeks;
- variance reduction;
- path-dependent options;
- Python bindings;
- profiling and benchmark executables;
- OpenMP.
