# Evidence Log

Nothing in this file is verified until an actual command, output, commit, test,
or benchmark is linked.

| Candidate claim | Required evidence | Current evidence | Status |
|---|---|---|---|
| Configured a C++20 project with CMake | Debug and Release configure/build logs; clean clone | Local and public-clean-clone Debug/Release checks passed; CI run `30621709642` passed four macOS/Linux matrix jobs on 2026-07-31 | Verified for scaffold only |
| Implemented Black–Scholes pricing | Source, derivation, independent reference tests | None | Locked |
| Implemented Monte Carlo pricing | Source, seed tests, confidence-interval tests, error study | None | Locked |
| Exposed C++ to Python | pybind11 source, clean install, Python tests | None | Locked |
| Parallelized with OpenMP | Serial profile, correctness tests, raw fixed-hardware timings | None | Locked |
| Achieved a measured speedup | Raw repeated timings and limitations | None | Locked |

## Evidence-entry template

```text
Date:
Candidate claim:
Repository tag/commit:
Files/symbols:
Command:
Raw output:
Test or benchmark:
Hardware/toolchain:
Limitations:
Safe wording:
Status: Locked/Candidate/Verified
```

## 2026-07-31 — Local scaffold verification

- Environment: Apple M1 arm64, macOS 26.0.1, Apple Clang 15.0.0, CMake 4.4.1.
- Debug configure: passed with testing and warnings-as-errors enabled.
- Debug build: passed without warnings.
- Debug CTest: the scaffold smoke test passed.
- Debug CLI: exited zero and reported that pricing is not implemented.
- Release configure: passed with testing and warnings-as-errors enabled.
- Release build: passed without warnings.
- Release CTest: the scaffold smoke test passed.
- Release CLI: exited zero and reported that pricing is not implemented.
- Isolated local clone: commit `a7f3fa1` configured, built, tested, and ran in Debug and Release; the generated build directories remained ignored.
- Public clone: commit `3856c45` configured, built, tested, and ran in Debug and Release from the public HTTPS URL; generated build directories remained ignored.
- GitHub Actions: [run `30621709642`](https://github.com/000zer000/high-performance-derivatives-pricing-risk-engine/actions/runs/30621709642) passed Ubuntu Debug, Ubuntu Release, macOS Debug, and macOS Release jobs for commit `3856c45`.
- Limitations: no pricing, completed validation rules, RNG, statistics, numerical-error study, or benchmark exists.
- Status: Verified for the build scaffold only. No quantitative-development claim is unlocked.
