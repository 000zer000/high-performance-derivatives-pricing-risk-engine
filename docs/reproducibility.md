# Reproducibility

## Current toolchain

- macOS 26.0.1;
- Apple M1 arm64;
- Apple Clang 15.0.0;
- CMake and CTest 4.4.1.

Sensitive device identifiers are deliberately excluded.

## v0.1.0 target contract

- caller-supplied unsigned 64-bit seed;
- one serial `std::mt19937_64` stream;
- exact path count recorded;
- compiler and standard-library environment recorded;
- same seed and fixed toolchain reproduce the same result.

The C++ standard does not require `std::normal_distribution` to produce
bitwise-identical sequences across different standard-library
implementations. Cross-platform and thread-count-independent bitwise identity
are not current claims.

## Evidence still required

- exact compiler command lines when numerical implementation begins;
- deterministic repeated-run output;
- clean-clone output from the public GitHub remote;
- a green GitHub Actions run;
- later benchmark protocol and raw results.

## Verified scaffold evidence — 2026-07-31

- Debug and Release configurations generated successfully.
- Both configurations built with warnings treated as errors.
- The single scaffold smoke test passed in both configurations.
- The placeholder CLI produced the same truthful status message in both configurations.
- Generated build files remained outside the source tree and ignored by Git.
- An isolated local clone of commit `a7f3fa1` passed the same Debug and Release checks.
- This is build reproducibility evidence only, not numerical reproducibility evidence.
