# High-Performance Derivatives Pricing & Risk Engine

Status: **correctness-first scaffold**. Pricing, risk, Python bindings,
parallelization, and performance benchmarks are not implemented or validated.

This public project is intended to build inspectable quantitative-development
evidence through tested numerical methods and measured performance. The words
“high-performance” describe the target, not a current claim.

## Current scaffold

The repository currently provides:

- a C++20 CMake project;
- a reusable `pricing_core` target with implementation placeholders;
- a `pricing_cli` executable that reports the scaffold status;
- one build smoke test registered with CTest;
- warning configuration for Apple Clang, Clang, GCC, and MSVC;
- GitHub Actions configuration for macOS and Linux;
- documentation and evidence-log placeholders.

It does **not** currently provide an option price, Greek, confidence interval,
Python interface, OpenMP implementation, error measurement, or benchmark.

## Planned v0.1.0 scope

- Black–Scholes analytical European call and put prices;
- serial European Monte Carlo under geometric Brownian motion;
- caller-supplied seed;
- sample standard error and 95% confidence interval;
- input validation;
- automated correctness tests;
- clean-clone documentation.

All later features remain gated by correctness evidence.

## Configure, build, and run the scaffold

Prerequisites:

- CMake 3.25 or newer;
- a C++20 compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/pricing_cli
```

These scaffold commands were verified locally on 31 July 2026 in both Debug
and Release configurations with compiler warnings treated as errors. The one
registered smoke test passed in each configuration. This verifies only the
build scaffold; it does not verify a pricing model.

The same configure, build, test, and CLI checks also passed from an isolated
local clone of commit `a7f3fa1`. A clean clone from the future public GitHub
remote and a GitHub Actions run are still pending.

## Evidence policy

No numerical accuracy, test count, timing, variance reduction, speedup, or
résumé claim is published until supported by saved commands and outputs.
See [`docs/evidence_log.md`](docs/evidence_log.md).

## Confidentiality

This repository uses mathematical models and synthetic parameters only. It
contains no employer data, code, schema, credential, screenshot, or
confidential business logic.

## License

MIT
