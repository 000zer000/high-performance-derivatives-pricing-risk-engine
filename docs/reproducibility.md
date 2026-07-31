# Reproducibility

## Random-draw contract

The engine has no wall-clock seed and no shared mutable random stream. A draw
is a pure function of caller seed, path index, and time-step index:

1. SplitMix64-style integer mixing creates two 64-bit values;
2. their upper 53 bits map to open-interval uniform values;
3. Box–Muller produces a standard-normal value.

Antithetic paths reuse the same addressed draw with the opposite sign.

This makes path generation independent of OpenMP scheduling. The random stream
is designed for reproducible simulation, not cryptography.

## Reduction contract

The independent observations are split into fixed blocks of 2,048. Values are
added in ascending path order within a block using Welford statistics. OpenMP
may compute blocks in any schedule, but completed block statistics are merged
in ascending block order. Tests require exact equality of price, standard
error, and interval bounds between serial and four-thread execution for both
European and path-dependent cases.

## Boundary of the claim

The same inputs, binary, compiler, math library, and seed reproduce exactly the
same result regardless of selected thread count. Cross-platform bitwise
identity is not claimed: `exp`, `log`, `cos`, and related floating operations
may differ across operating systems and libraries.

Reproducibility also does not mean identical runtime. CPU load, thermal state,
frequency scaling, and OpenMP runtime behavior affect timings.

## Audited local environment

- macOS 26.0.1, arm64;
- Apple M1, 8 physical/logical cores, 8 GiB RAM;
- Apple Clang 15.0.0;
- CMake/CTest 4.4.1;
- Python 3.13.1;
- libomp 22.1.8;
- pybind11 3.0.4.

No hardware serial number, UUID, user email, or credential is recorded.

## Verified local gates on 2026-07-31

- Debug C++/OpenMP: 60/60 CTest cases;
- Release C++/OpenMP: 60/60 CTest cases;
- Debug Python-enabled: 61/61 CTest cases;
- Debug AddressSanitizer + UndefinedBehaviorSanitizer, OpenMP off: 59/59
  CTest cases with no reported finding;
- fixed-seed serial/OpenMP numerical equality tests passed;
- benchmark and convergence commands completed and raw relevant outputs were
  committed.

Public CI and clean-clone results are added only after they actually pass.
