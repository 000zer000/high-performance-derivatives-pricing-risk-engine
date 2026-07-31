# Reproducibility

## Audited local toolchain

- macOS 26.0.1;
- Apple M1 arm64;
- Apple Clang 15.0.0;
- CMake and CTest 4.4.1;
- Release and Debug builds with project warnings treated as errors.

Sensitive device identifiers are deliberately excluded.

## v0.1.0 contract

- caller-supplied unsigned 64-bit seed;
- one serial `std::mt19937_64` stream;
- exact path count recorded in each result;
- same seed, inputs, binary, compiler, and standard library reproduce the same
  output;
- no hidden wall-clock seed;
- no parallel scheduling in v0.1.0.

The C++ standard fixes the `std::mt19937_64` engine but does not require
`std::normal_distribution` to map engine values identically across every
standard-library implementation. Cross-platform bitwise equality is therefore
not claimed.

## Verified local commands — 2026-07-31

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON -DDPR_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DDPR_WARNINGS_AS_ERRORS=ON
cmake --build build-release --parallel
ctest --test-dir build-release --output-on-failure
```

Actual result: 36/36 CTest cases passed in each configuration.

The Release Monte Carlo call command with 250,000 paths and seed 20,260,731
was executed twice. Both complete `key=value` outputs were byte-for-byte
identical. Its recorded result was:

```text
price=10.405958339551766
standard_error=0.029309857309524682
ci95_lower=10.348511019225098
ci95_upper=10.463405659878434
```

## Toolchain portability observation

Apple libc++ paired with the audited Apple Clang 15 installation does not
provide floating-point `std::from_chars`. The first warning-clean CLI build
failed at that call. Floating CLI arguments therefore use `std::strtod` with
full-string and range checks; unsigned path counts and seeds still use integer
`std::from_chars`.

## Verified public gate — 2026-07-31

- Commit `80082f2` passed [GitHub Actions run `30624203320`](https://github.com/000zer000/high-performance-derivatives-pricing-risk-engine/actions/runs/30624203320).
- Ubuntu Debug, Ubuntu Release, macOS Debug, and macOS Release each passed
  36/36 CTest cases.
- A fresh clone from the public HTTPS URL configured and built Release with
  warnings-as-errors, passed 36/36 tests, and reproduced both README examples.
- Generated build files remained ignored and the public clone stayed Git-clean.

## Evidence still required after v0.1.0

- multi-seed path-count convergence results;
- later variance-reduction, Python, profiling, and OpenMP evidence.
