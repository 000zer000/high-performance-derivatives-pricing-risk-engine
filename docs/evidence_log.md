# Evidence Log

Nothing in this file is verified until an actual command, output, commit, test,
or benchmark is linked.

| Candidate claim | Required evidence | Current evidence | Status |
|---|---|---|---|
| Configured a C++20 project with CMake | Debug and Release configure/build logs; clean clone | Local Debug/Release builds and scaffold smoke tests passed with warnings as errors on 2026-07-31; CI and clean clone pending | Candidate |
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
- Limitations: no pricing, validation rules, RNG, statistics, numerical-error study, CI run, clean clone, or benchmark exists.
- Status: Candidate. Public clean-clone and CI evidence are still required.
