# Evidence Log

Nothing is a résumé claim until its code, tests, measurements, documentation,
public CI, and limitations are present. “Local candidate” means the local gates
passed but public verification is still pending.

| Candidate claim | Evidence | Status |
|---|---|---|
| Built a C++20/CMake pricing library | `CMakeLists.txt`, warning-clean Debug/Release, CI workflow, 57 local C++ tests | Local candidate |
| Implemented Black–Scholes European pricing | `src/black_scholes.cpp`, 9 analytical tests, independent Python reference, parity/boundaries | Local candidate |
| Implemented analytical delta/gamma/vega | `src/greeks.cpp`, 5 tests including independent values and finite differences | Local candidate |
| Priced European, arithmetic Asian, and down-and-out contracts with Monte Carlo | `src/monte_carlo.cpp`, validation, deterministic limits, seed and error-screen tests | Local candidate |
| Reported standard errors and 95% intervals | Welford implementation/tests, interval formula test, 30-seed CSV and limitations | Local candidate |
| Implemented antithetic variates | pair-average implementation, effective-sample test, frozen SE comparison, multi-seed results | Local candidate |
| Implemented a justified control variate | analytical beta derivation, European-only validation, SE/error tests, multi-seed results | Local candidate |
| Exposed C++ pricing to Python | `python/bindings.cpp`, 7 Python unittest methods inside the binding CTest | Local candidate |
| Parallelized fixed path blocks with OpenMP | serial profile, fixed-order design, exact equality tests, OpenMP CLI/Python tests | Local candidate |
| Measured a fixed-hardware speedup | five raw repetitions: 5.517x European and 5.505x Asian medians on Apple M1 | Local candidate |
| Measured convergence/error | independent Python reference, 30 seeds x 4 path counts x 3 estimators, committed CSV | Local candidate |
| Used sanitizers | local ASan+UBSan build, 59/59 tests, no reported finding | Local candidate |

## Local verification record — 2026-07-31

### Correctness

- Debug and Release C++/OpenMP builds were warning-clean.
- 60/60 CTest cases passed in each C++ build.
- The pybind11 Debug build passed 61/61 CTest cases, including seven Python
  unittest methods.
- The OpenMP-disabled ASan+UBSan build passed 59/59 CTest cases with no
  sanitizer finding.
- Serial and four-thread results matched exactly in European and 24-step Asian
  tests because random draws and reduction order are fixed.

Test counts are named invocations, not a coverage percentage and not proof of
production readiness.

### Serial profile

Command:

```bash
build/release/serial_profile 3000000 128
sample serial_profile 5 -file /tmp/dpr_serial_profile_20260731.txt
```

Of 3,766 stacks, 2,746 top frames (approximately 72.9%) were directly in
`exp`, `log`, `cos`, or `normal_draw`; additional frames were inside the math
library. This supports path-block parallelization. See the privacy-preserving
raw extract in `benchmarks/results/serial-profile-2026-07-31.txt`.

### Fixed-hardware benchmark

Command:

```bash
build/release/pricing_benchmark 5000000 300000 64 5 8
```

| Workload | Serial median | 8-thread median | Speedup | Same result |
|---|---:|---:|---:|---|
| European plain, 5M paths | 0.383474209 s | 0.069511125 s | 5.51673144x | yes |
| Asian antithetic, 300k paths, 64 steps | 1.33612417 s | 0.242718041 s | 5.50484077x | yes |

Hardware: Apple M1, 8 cores, 8 GiB, macOS 26.0.1, Apple Clang 15.0.0,
libomp 22.1.8. Every timed repetition is in
`benchmarks/results/2026-07-31-apple-m1.txt`.

### Convergence observation

Thirty fixed seeds were run at 2,000, 20,000, 200,000, and 2,000,000 paths for
plain, antithetic, and control-variate estimators. Plain RMSE fell from
`0.3486720073` to `0.0102551025`. At 2,000,000 paths, antithetic RMSE was
`0.0078536772` and control-variate RMSE was `0.0045638660`.

Observed interval coverage ranged from 83.3% to 100% across the twelve rows.
Thirty trials are too few for a formal coverage claim, and the study covers
one European call parameter set only.

## Résumé wording gate

After public CI, a public clean clone, and the `v1.0.0` tag pass, the evidence
supports this narrow statement:

> Built a C++20 derivatives pricing engine with Black–Scholes analytics,
> reproducible Monte Carlo for European, arithmetic-Asian, and discretely
> monitored down-and-out options, pybind11 bindings, variance reduction, and
> deterministic OpenMP block parallelism; measured 5.52x and 5.50x median
> speedups on fixed 8-core Apple M1 workloads and validated numerical error in
> a committed 30-seed study.

Do not shorten this into “production pricing system,” “continuous barrier
model,” “cross-platform bitwise reproducibility,” or an unqualified speedup.
