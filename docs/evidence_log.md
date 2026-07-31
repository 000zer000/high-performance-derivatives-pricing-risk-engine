# Evidence Log

Nothing is a résumé claim until its required code, tests, command output, and
limitations are present. “Candidate” means implemented and locally tested but
not through every release gate.

| Candidate claim | Required evidence | Current evidence | Status |
|---|---|---|---|
| Configured a C++20 project with CMake | Warning-clean builds, CI, public clean clone | Local builds, four-job CI, and public clean clone pass at `80082f2` | Verified for v0.1 scope |
| Implemented Black–Scholes European pricing | Source, derivation, independent references, boundaries, CI | Nine named cases pass locally and in four CI jobs; public clone passes | Verified for v0.1 scope |
| Implemented reproducible serial European Monte Carlo | Source, seed/metadata/CI tests, error screen, CI | Nine named cases, repeated local output, four-job CI, and public clone pass | Verified within documented boundary |
| Computed confidence intervals | Sample-variance/SE tests and interval formula test | Statistics and interval cases pass locally, in CI, and from public clone | Verified for implemented estimator |
| Exposed C++ to Python | pybind11 source, clean install, Python tests | None | Locked |
| Parallelized with OpenMP | Serial profile, correctness tests, raw fixed-hardware timings | None | Locked |
| Achieved a measured speedup | Raw repeated timings and limitations | None | Locked |

## 2026-07-31 — v0.1.0 local implementation verification

### Repository state

- Feature commits:
  - `6396760` — option validation and running statistics;
  - `8fa240d` — Black–Scholes pricing;
  - `7d4bceb` — serial seeded Monte Carlo;
  - `bc37581` — analytical and Monte Carlo CLI.
- Evidence commit: `80082f2` — formulas, commands, outputs, and limitations.
- Build configuration: C++20, extensions disabled, warnings-as-errors enabled.
- Local Debug CTest: 36/36 passed.
- Local Release CTest: 36/36 passed.
- GitHub Actions run `30624203320`: 36/36 passed in each of Ubuntu Debug,
  Ubuntu Release, macOS Debug, and macOS Release.
- Public clean clone of `80082f2`: Release build and 36/36 tests passed; both
  README examples reproduced; generated files remained ignored.

### Analytical evidence

Files and symbols:

- `src/black_scholes.cpp` — `black_scholes_price`;
- `tests/test_black_scholes.cpp` — nine named reference, parity, boundary, and
  non-negativity cases;
- `docs/mathematics.md` — assumptions, formula, Python recomputation command,
  references, and tolerances.

Frozen analytical outputs:

```text
call=10.450583572185565
put=5.5735260222569734
```

The put differs from the independently recorded Python value by approximately
`2.7e-15`, far below the declared `1e-10` absolute tolerance.

### Monte Carlo evidence

Files and symbols:

- `src/monte_carlo.cpp` — `price_european_monte_carlo`;
- `src/running_statistics.cpp` — Welford update, sample variance, SE, merge;
- `tests/test_monte_carlo.cpp` — seed, metadata, interval, boundary, and error
  screens;
- `tests/test_running_statistics.cpp` — four deterministic statistics cases.

Frozen inputs: spot 100, strike 100, rate 0.05, volatility 0.20, maturity 1,
250,000 paths, seed 20,260,731.

| Type | Price | Standard error | 95% interval | Absolute analytical error | Error / SE |
|---|---:|---:|---:|---:|---:|
| Call | 10.405958339551766 | 0.029309857309524682 | [10.348511019225098, 10.463405659878434] | 0.04462523263379836 | 1.5225332612 |
| Put | 5.5710527461979824 | 0.017306743952028705 | [5.5371315280520061, 5.6049739643439587] | 0.0024732760589909475 | 0.1429082250 |

The complete Release call output was identical in two consecutive executions
on the audited toolchain.

### Limitations and safe wording

- This is one parameter case and one seed, not a convergence study.
- A 95% interval is an estimator uncertainty statement, not a guarantee that
  every interval contains the analytical value.
- No timing was taken and no performance claim is unlocked.
- `std::normal_distribution` prevents a cross-platform bitwise claim.
- The v0.1.0 tag is created only after this verification update also passes CI.

Safe wording supported by the verified v0.1.0 implementation:

> Implemented and tested Black–Scholes and reproducible serial Monte Carlo
> pricing for no-dividend European calls and puts in C++20, including input
> validation, standard errors, confidence intervals, CMake, CLI, and CI.

This wording does not claim production readiness, general numerical accuracy,
parallelism, Python integration, Greeks, or high performance.
