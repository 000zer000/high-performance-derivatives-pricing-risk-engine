# Project Defense Programme

This guide teaches and tests the finished `v1.0.0` system. It is not an
implementation checklist. The goal is to explain every public claim, trace it
to source and evidence, and state its boundary without exaggeration.

## How to use this programme

Complete four passes for each round:

1. Read the explanation and follow every referenced source file.
2. Close the repository and explain the topic aloud in your own words.
3. Draw the relevant formula or execution flow from memory.
4. Answer the defense questions without notes, then verify your answer.

A round is passed only when the answer includes the mechanism, the engineering
choice, the evidence, and at least one limitation. Memorizing a benchmark
number without being able to explain the workload and measurement method is not
a pass.

## The 90-second project explanation

> This is a C++20 derivatives pricing library with a command-line client and a
> thin pybind11 interface. It implements no-dividend Black-Scholes prices and
> analytical delta, gamma, and vega, plus reproducible Monte Carlo estimators
> for European, arithmetic-Asian, and discretely monitored down-and-out
> options. The simulation uses path-indexed random draws, Welford statistics,
> antithetic sampling, and an analytically derived European stock control
> variate. I profiled the serial workload before parallelizing fixed
> 2,048-observation blocks with OpenMP, then merged blocks in a fixed order so
> serial and parallel results are exactly equal on the same binary and math
> library. On one recorded 8-core Apple M1 setup, five post-warm-up repetitions
> produced median speedups of 5.517x for a 5-million-path European workload and
> 5.505x for a 300,000-path, 64-step Asian workload. CTest, pybind11 tests,
> Linux/macOS CI, sanitizers, a 30-seed convergence study, raw timings, and
> explicit limitations support the claims. It is a portfolio engine, not a
> production trading platform.

Do not recite this mechanically. Be able to replace every sentence with a
source file, test, or saved measurement.

## Round 1 — Architecture and execution flow

### Component map

```text
pricing_cli ---------\
pybind11 module ------+--> pricing_core --> validation
CTest executables ----+                  |-> Black-Scholes and Greeks
benchmark drivers ----+                  |-> deterministic random draws
convergence script ---/                  |-> contract payoff simulation
                                          |-> fixed-block execution
                                          \-> running statistics/results
```

`pricing_core` is the static C++ library and the sole owner of financial and
numerical logic. Its public types live under `include/derivatives/`; their
implementations live under `src/`.

- `app/pricing_cli.cpp` parses text, builds typed inputs, calls the public core,
  and prints `key=value` results.
- `python/bindings.cpp` maps the same C++ types and functions into the
  `derivatives_engine` module. It does not duplicate a pricing formula.
- `tests/` calls the public API and registers named cases with CTest.
- `benchmarks/` calls the same public API used by clients.
- `experiments/convergence.py` treats the CLI as an external executable and
  uses an independently written Python Black-Scholes reference.

### Monte Carlo call path

```text
CLI or Python input
  -> construct contract and MonteCarloConfig
  -> validate contract and configuration
  -> choose deterministic shortcut or stochastic estimator
  -> map requested paths to independent observations
  -> divide observations into fixed blocks
  -> evaluate blocks serially or with OpenMP
  -> merge block-level Welford states in ascending block order
  -> calculate mean, standard error, and 95% interval
  -> return MonteCarloResult
```

Validation is inside every public pricing entry point, not only in the CLI.
That matters because Python and future C++ clients cannot bypass domain checks.
Standard C++ exceptions cross the pybind11 boundary as Python exceptions.

### Defense questions

1. Why is the CLI not allowed to own a Black-Scholes formula?
2. What changes if a caller invokes the C++ library directly rather than the
   CLI?
3. Why do benchmarks call the public core API?
4. Trace a Python Asian-option request from object construction to its returned
   confidence interval.

Evidence: `CMakeLists.txt`, `app/pricing_cli.cpp`, `python/bindings.cpp`,
`docs/architecture.md`, and `src/option.cpp`.

## Round 2 — Black-Scholes pricing and analytical Greeks

The model assumes a non-dividend-paying asset, constant risk-free rate `r`,
constant volatility `sigma`, maturity `T`, and European exercise. Define

```text
d1 = [ln(S/K) + (r + sigma^2/2)T] / [sigma sqrt(T)]
d2 = d1 - sigma sqrt(T)
```

With `N` the standard-normal CDF, the prices are

```text
call = S N(d1) - K exp(-rT) N(d2)
put  = K exp(-rT) N(-d2) - S N(-d1)
```

The implementation computes `log(S) - log(K)` rather than forming `S/K`
first. `std::erfc` supplies the CDF. The result is checked for finiteness and
clamped at zero to suppress tiny negative roundoff.

The analytical Greeks are

```text
call delta = N(d1)
put delta  = N(d1) - 1
gamma      = n(d1) / [S sigma sqrt(T)]
vega       = S n(d1) sqrt(T)
```

where `n` is the standard-normal density. Delta is the local price sensitivity
to one unit of spot. Gamma is the local change in delta per unit of spot. The
implemented vega is the derivative with respect to a volatility change of
`1.0`; divide it by 100 to quote the change for one volatility percentage
point.

At `T = 0`, pricing returns intrinsic value. At `sigma = 0`, pricing returns
the discounted deterministic payoff. Analytical Greeks reject both cases
because the regular formulas divide by `sigma sqrt(T)` and the limiting Greek
can be discontinuous at the strike.

### Defense questions

1. Explain the economic meaning of the two terms in the call formula.
2. Why is the put delta the call delta minus one?
3. What unit does this engine's vega use?
4. How do put-call parity, independent reference values, and finite differences
   test different failure modes?
5. Why does a zero-volatility price exist while the regular Greek formula is
   rejected?

Evidence: `src/black_scholes.cpp`, `src/greeks.cpp`,
`tests/test_black_scholes.cpp`, `tests/test_greeks.cpp`, and
`docs/mathematics.md`.

## Round 3 — Risk-neutral simulation and the three contracts

Under the risk-neutral measure, geometric Brownian motion satisfies

```text
dS_t = r S_t dt + sigma S_t dW_t
```

The exact constant-parameter step over `Delta t` is

```text
S_(t+Delta t) = S_t exp[(r - sigma^2/2)Delta t
                        + sigma sqrt(Delta t) Z]
```

The drift is `r`, rather than a forecast of the asset's real-world expected
return, because a derivative price is the discounted risk-neutral expectation
of its payoff. Every stochastic price therefore estimates

```text
V_0 = exp(-rT) E_Q[payoff].
```

### European option

One exact step generates `S_T`, followed by

```text
call payoff = max(S_T - K, 0)
put payoff  = max(K - S_T, 0).
```

The analytical Black-Scholes price is available as a reference for tests and
the convergence experiment.

### Arithmetic-average Asian option

The path uses `m` equally spaced future observations. The arithmetic average
is

```text
A = (S_t1 + ... + S_tm) / m.
```

The initial spot `S_0` is intentionally excluded. The discounted call or put
payoff is based on `A`, not `S_T`. This path dependence prevents replacing the
simulation with the European terminal distribution.

### Discrete down-and-out option

The engine checks the initial spot and every one of `m` equally spaced future
observations. If any checked value is less than or equal to the barrier, the
payoff is zero. Otherwise, it is the discounted European terminal payoff. The
rebate is zero.

This is a discretely monitored contract. A continuously monitored barrier can
be crossed between observation dates, so this implementation has monitoring
bias relative to a continuous-barrier model.

### Defense questions

1. Why is the GBM drift `r - sigma^2/2` inside the exponential but `r` in the
   stochastic differential equation?
2. Why may the European contract use one step while the Asian contract cannot?
3. Does the Asian average include today's spot?
4. Does touching the barrier knock the option out? Is the initial spot tested?
5. Name the source of error in a barrier estimate that increasing only the
   number of Monte Carlo paths will not remove.

Evidence: `src/monte_carlo.cpp`, `src/option.cpp`,
`tests/test_monte_carlo.cpp`, and `tests/test_path_dependent.cpp`.

## Round 4 — Online statistics and uncertainty

For observation `x_n`, Welford's online update stores count `n`, mean, and the
sum of squared deviations `M2`:

```text
n     <- n + 1
delta <- x - mean
mean  <- mean + delta/n
M2    <- M2 + delta (x - new_mean)
```

The sample variance and standard error are

```text
s^2 = M2/(n - 1)
SE  = s/sqrt(n).
```

This avoids the cancellation-prone computation `sum(x^2) - n mean^2` and does
not store every payoff.

Two Welford states can be merged. If groups `a` and `b` have counts `n_a`,
`n_b`, means `m_a`, `m_b`, and squared-deviation sums `M2_a`, `M2_b`, then

```text
delta = m_b - m_a
n = n_a + n_b
mean = m_a + delta n_b/n
M2 = M2_a + M2_b + delta^2 n_a n_b/n.
```

The reported interval is

```text
estimate +/- 1.96 SE.
```

It is a large-sample normal approximation for Monte Carlo sampling
uncertainty. It is not an exact finite-sample guarantee and does not include
model risk, parameter uncertainty, or time-monitoring bias.

### Antithetic observations and effective sample count

For each normal vector `Z`, the engine also evaluates `-Z` and adds one
pair-average observation:

```text
Y_pair = [Y(Z) + Y(-Z)]/2.
```

The two payoffs are dependent. Treating them as two independent observations
would understate uncertainty. For `paths = N`, antithetic sampling therefore
reports `effective_samples = N/2`. The path count must be even and at least
four. Antithetic sampling can reduce variance when paired payoffs are
negatively correlated, but the benefit is payoff- and parameter-dependent.

### Defense questions

1. Why is Welford preferable to accumulating both the sum and squared sum?
2. What does `n - 1` mean in the variance calculation?
3. Why is the antithetic effective sample count half the requested path count?
4. What uncertainty does the interval omit?
5. Why can a nominal 95% procedure miss the reference more or less than 5% in
   a finite 30-trial experiment?

Evidence: `src/running_statistics.cpp`, `src/monte_carlo.cpp`,
`tests/test_running_statistics.cpp`, and `tests/test_monte_carlo.cpp`.

## Round 5 — The European control variate

The control is discounted terminal stock:

```text
X = exp(-rT) S_T
E[X] = S
Var(X) = S^2 [exp(sigma^2 T) - 1].
```

If `Y` is the discounted option payoff, the adjusted observation is

```text
Y_cv = Y - beta (X - S)
beta = Cov(Y, X) / Var(X).
```

Subtracting a zero-mean quantity preserves the estimator's expectation. The
variance-minimizing beta depends on covariance: a control strongly related to
the payoff removes more noise.

The engine does not estimate beta on the same simulation sample. It derives
beta analytically from truncated lognormal moments. With
`v = sigma sqrt(T)` and Black-Scholes price `P`, it uses

```text
call: E[YX] = S^2 exp(v^2) N(d1 + v)
              - K S exp(-rT) N(d1)

put:  E[YX] = K S exp(-rT) N(-d1)
              - S^2 exp(v^2) N(-d1 - v)

Cov(Y,X) = E[YX] - P S.
```

Because the derivation is specific to the European payoff and terminal-stock
control, Asian and barrier requests are rejected. The engine does not silently
fall back or invent a beta.

### Defense questions

1. Why does `E[exp(-rT)S_T]` equal today's spot under this model?
2. Why does subtracting `beta(X-S)` not change the target expectation?
3. What is the advantage of an analytical beta over a beta fitted on the same
   sample?
4. Why is the control variate not exposed for the two path-dependent contracts?
5. Does a control variate guarantee the same variance reduction for every
   parameter set?

Evidence: `src/monte_carlo.cpp`, `docs/mathematics.md`, and
`tests/test_variance_parallel.cpp`.

## Round 6 — Deterministic random draws and reproducibility

`normal_draw(seed, path, step)` is a pure indexed calculation. It combines the
three integer coordinates with SplitMix64-style mixing, produces two 64-bit
values, and maps each value's upper 53 bits to

```text
U = (mantissa + 0.5) / 2^53.
```

The `+0.5` puts `U` strictly inside `(0,1)`. That prevents `log(0)` in the
Box-Muller transform:

```text
Z = sqrt[-2 log(U1)] cos(2 pi U2).
```

Path indexing avoids a shared mutable generator. A thread does not consume
"the next" random value; it calculates the value assigned to a specific path
and step. OpenMP scheduling therefore cannot change which random variate a path
receives. Antithetic simulation reuses the indexed draw with the opposite
sign.

The claim has a strict boundary: the same inputs, seed, binary, compiler, and
math library reproduce exactly across thread counts. Cross-platform bitwise
identity is not claimed because implementations of `exp`, `log`, `cos`, and
other floating operations may differ. The mixer is for deterministic
simulation, not cryptography.

### Defense questions

1. Why use the upper 53 bits when building a `double` uniform?
2. Why must the uniforms be in an open interval for Box-Muller?
3. How does path indexing differ from one pseudorandom engine per thread?
4. What reproducibility claim is supported, and which stronger claim is not?
5. Is this random stream suitable for passwords or cryptographic keys?

Evidence: `src/deterministic_random.cpp`, `docs/reproducibility.md`, and the
fixed-seed equality cases in `tests/`.

## Round 7 — Fixed blocks and OpenMP

The independent observation count is divided into blocks of 2,048. For plain
and control sampling, one observation is one path. For antithetic sampling,
one observation is a two-path pair, so a full block represents 4,096 raw paths.

Within each block, observations enter a private Welford accumulator in
ascending index order. OpenMP distributes block indices with
`schedule(static)`. Each block writes to its own statistics and exception slot,
so the hot loop needs no shared lock. After the parallel region, the caller
checks failures and merges block states in ascending block order.

The fixed reduction tree is the important reproducibility mechanism. A naïve
OpenMP reduction would combine floating-point partial sums in an order that can
depend on the thread count or schedule. Floating-point addition is not
associative, so that could change low bits. Here, the calculation and merge
order are fixed even if block completion order differs.

C++ exceptions do not propagate safely through an OpenMP parallel boundary.
The worker therefore captures an `exception_ptr` for its block, and the serial
post-processing phase rethrows it.

### Defense questions

1. Why is the block size expressed in observations rather than raw paths?
2. What part of the design gives exact serial/parallel equality on one binary?
3. Does `schedule(static)` alone guarantee the fixed numerical result?
4. Where can threads write, and why is there no mutex in the payoff loop?
5. Why are exceptions stored and rethrown later?

Evidence: `simulate_samples` in `src/monte_carlo.cpp`,
`tests/test_variance_parallel.cpp`, and `docs/architecture.md`.

## Round 8 — Profiling and benchmark defense

### Why profile first

The serial profile used a 3,000,000-path, 128-step arithmetic-Asian antithetic
workload. Of 3,766 sampled stacks, 2,746 top frames, approximately 72.9%, were
directly in `exp`, `log`, `cos`, or `normal_draw`, with more time inside the
math library below those frames. The expensive work was therefore path-local
numerical computation, which justified block-level shared-memory parallelism.

This is sampled evidence from one workload, not a universal statement about
every contract or machine.

### Benchmark design

The committed benchmark uses a Release build, `std::chrono::steady_clock`, one
warm-up call per execution policy, five timed repetitions, and alternating
serial-first/parallel-first order to reduce ordering bias. It records every
timing, reports the median, and checks that every serial/parallel result pair
has exactly equal price, standard error, and interval bounds.

On Apple M1, 8 cores, 8 GiB RAM, Apple Clang 15.0.0, and libomp 22.1.8:

| Workload | Serial median | 8-thread median | Median speedup |
|---|---:|---:|---:|
| European plain, 5,000,000 paths | 0.383474209 s | 0.069511125 s | 5.51673144x |
| Asian antithetic, 300,000 paths, 64 steps | 1.33612417 s | 0.242718041 s | 5.50484077x |

Median speedup is `median(serial times) / median(parallel times)`. A median is
less sensitive than a mean to one unusually slow repetition, but five
repetitions are still a small study.

An 8-thread run is not expected to be 8x faster. Serial setup and ordered
merging, OpenMP scheduling overhead, load imbalance in early-exiting barrier
paths, shared hardware resources, memory effects, CPU frequency and thermal
behavior, and other machine load all limit and perturb scaling. The results are
evidence for these two fixed workloads on the recorded machine, not a portable
speedup guarantee.

### Defense questions

1. What profiling observation justified parallelizing over blocks?
2. Why warm up both policies and alternate their timing order?
3. Why report a median and all raw observations?
4. Define the exact numerator and denominator of the reported speedup.
5. Give at least four reasons that 8 threads did not produce 8x speedup.
6. Why must numerical equality be checked inside a performance benchmark?

Evidence: `benchmarks/serial_profile.cpp`,
`benchmarks/results/serial-profile-2026-07-31.txt`,
`benchmarks/pricing_benchmark.cpp`, and
`benchmarks/results/2026-07-31-apple-m1.txt`.

## Round 9 — CMake, tests, Python, CI, and sanitizers

`CMakeLists.txt` creates a C++20 static library named `pricing_core`, links the
CLI and optional consumers to it, exports only the public include directory,
and enables position-independent code for linking into the Python extension.
Project warnings include `-Wall`, `-Wextra`, `-Wpedantic`, `-Wconversion`, and
`-Wshadow` outside MSVC, with an option to promote warnings to errors.

The important configuration switches are:

- `BUILD_TESTING`: register CTest cases;
- `DPR_WARNINGS_AS_ERRORS`: fail the build on project warnings;
- `DPR_ENABLE_OPENMP`: detect and link an optional OpenMP runtime;
- `DPR_BUILD_PYTHON`: require pybind11 and build `derivatives_engine`;
- `DPR_BUILD_BENCHMARKS`: build benchmark and profiling drivers.

CTest is the test orchestrator. Separate executables cover option validation,
Black-Scholes, Greeks, running statistics, Monte Carlo, path-dependent
contracts, variance reduction, OpenMP equality, and CLI behavior. When Python
is enabled, CTest also runs `python/tests/test_bindings.py` with the built module
on `PYTHONPATH`.

The pybind11 layer exposes enums, input structs, output structs, pricing
functions, and `openmp_available()`. It stays thin so Python and C++ cannot
quietly disagree about a formula.

GitHub Actions runs four main jobs: Debug and Release on Ubuntu and macOS. Each
installs pybind11, enables tests, OpenMP, Python, benchmarks, and warnings as
errors, then builds and runs CTest. A fifth Ubuntu Debug job enables
AddressSanitizer and UndefinedBehaviorSanitizer. OpenMP is deliberately off in
that sanitizer job, so the sanitizer result covers the serial tested paths and
does not prove the OpenMP runtime itself free of defects.

The recorded local gates were 60/60 Debug and Release C++/OpenMP tests, 61/61
Python-enabled tests, and 59/59 OpenMP-disabled sanitizer tests with no reported
finding. These are named test invocations, not a code-coverage percentage or a
proof of production readiness. Both the documented verification revision and
the `v1.0.0` tag passed the five public CI jobs.

### Defense questions

1. Why does every front end link the same `pricing_core` target?
2. What does position-independent code enable here?
3. What is CTest's role compared with the individual C++ test executables?
4. What exactly do the five CI jobs cover?
5. What does the sanitizer job not cover?
6. Why would claiming “61 tests means full coverage” be wrong?

Evidence: `CMakeLists.txt`, `tests/CMakeLists.txt`, `python/bindings.cpp`,
`python/tests/test_bindings.py`, `.github/workflows/ci.yml`, and the linked
GitHub Actions runs.

## Round 10 — Convergence and measured pricing error

The convergence script independently implements the European call reference
with Python `math.erf`; it does not ask the C++ Black-Scholes function for the
answer. For each of four path counts (`2,000`, `20,000`, `200,000`, and
`2,000,000`) and three estimators (plain, antithetic, control), it runs 30 fixed
seeds. That is 360 pricing runs for one frozen parameter set.

For estimates `V_i`, analytical reference `V`, and `R = 30` trials:

```text
error_i = V_i - V
bias = mean(V_i) - V
RMSE = sqrt[mean(error_i^2)]
empirical standard deviation = sample standard deviation of the V_i
mean reported SE = mean of the 30 within-run standard errors
coverage = fraction of reported intervals containing V.
```

Bias measures average signed error. RMSE combines variance and squared bias and
penalizes errors regardless of sign. Empirical standard deviation measures how
the estimates varied across seeds. Mean reported SE is the engine's average
within-run uncertainty estimate; comparing it with empirical standard
deviation checks calibration. Coverage counts whether each interval contains
the independent analytical reference.

Observed plain RMSE fell from `0.3486720073` at 2,000 paths to `0.0102551025`
at 2,000,000 paths. At 2,000,000 paths, antithetic RMSE was `0.0078536772` and
control-variate RMSE was `0.0045638660`. Coverage across the twelve rows ranged
from 83.3% to 100%.

Those observations are consistent with convergence and useful variance
reduction for the frozen case. They are not proof of a theoretical convergence
rate, universal variance reduction, or exact 95% coverage: 30 trials and one
European call parameter set are too limited for those claims.

### Defense questions

1. Why must the convergence reference be independently implemented?
2. Distinguish bias, RMSE, empirical standard deviation, and reported SE.
3. What would well-calibrated standard errors look like across many trials?
4. How is interval coverage calculated?
5. Why can the table support a narrow result but not a universal accuracy
   claim?

Evidence: `experiments/convergence.py`,
`experiments/results/convergence-2026-07-31.csv`, `docs/mathematics.md`, and
`docs/evidence_log.md`.

## Complete limitation register

State these before an interviewer has to discover them.

### Financial model and contracts

- Rates and volatility are constant; the underlying pays no dividends.
- Exercise is European only. There is no American or Bermudan exercise logic.
- There is no calibration, volatility surface, stochastic volatility,
  stochastic rates, or live market-data integration.
- The Asian contract is an arithmetic average of equally spaced future
  observations and excludes the initial spot.
- The down-and-out contract is discretely monitored, includes the initial
  spot, knocks out on `spot <= barrier`, and pays zero rebate.
- Discrete barrier monitoring has bias relative to continuous monitoring; more
  paths reduce sampling error but do not remove that monitoring difference.
- Greeks are analytical delta, gamma, and vega only. Theta, rho, Monte Carlo
  Greeks, and path-dependent Greeks are absent.

### Statistical and numerical claims

- The 95% interval is a normal approximation for sampling error only. It omits
  model risk, parameter uncertainty, and barrier-monitoring error.
- Antithetic variance reduction is case-dependent, not guaranteed.
- The analytical stock control variate is implemented only for European
  options; unsupported path-dependent requests are rejected.
- The convergence dataset has 30 seeds, four path counts, three estimators, and
  one European call parameter set. It is finite evidence, not a formal proof of
  rate, unbiasedness, or exact coverage.
- Same-binary reproducibility does not imply cross-platform bitwise identity;
  compilers and math libraries can differ.
- The SplitMix64-style indexed stream is deterministic simulation machinery,
  not a cryptographic generator.

### Performance and verification claims

- The speedups apply only to the two recorded workloads on one Apple M1 setup
  on 2026-07-31 with five timed repetitions after warm-up.
- No cloud-runner or cross-compiler speed claim is made.
- Runtime varies with system load, thermals, frequency scaling, the OpenMP
  runtime, and other hardware/software conditions. Identical numerical output
  does not imply identical runtime.
- The profile is sampled evidence from one serial workload.
- The sanitizer build disables OpenMP; it does not sanitize the parallel
  runtime path in that configuration.
- Test counts are named invocations, not a code-coverage percentage and not
  proof that all inputs or platforms are correct.

### Product boundary

- This is a portfolio pricing engine, not production trading, valuation, or
  enterprise risk infrastructure.
- It has no order management, portfolio aggregation, calibration service,
  market-data service, persistence, networking, authentication, authorization,
  audit controls, regulatory controls, or transaction-cost model.
- It has no GPU or distributed-computing implementation.
- The Python extension is a local CMake/pybind11 build, not a published Python
  package with a stable external compatibility promise.

## Final hostile-interview drill

Answer each in two minutes or less, then point to evidence.

1. “You call this high performance. What did you measure before and after?”
2. “Why should I believe parallel execution did not change your answer?”
3. “Your random generator is deterministic. Is it statistically or
   cryptographically perfect?”
4. “Why is your 95% interval not a guarantee?”
5. “What makes the control variate mathematically justified?”
6. “Why does antithetic sampling have fewer effective samples?”
7. “What error remains in the barrier price with infinitely many simulated
   paths but the same monitoring grid?”
8. “How independent is your convergence reference?”
9. “What does the sanitizer result actually establish?”
10. “What would have to change before this could be called production-ready?”

For every answer, use this structure:

```text
Claim -> mechanism -> evidence -> measured result -> limitation.
```

## Definition of defense-ready

You are ready to discuss the project on a résumé only when you can:

- draw the architecture and Monte Carlo execution flow without notes;
- derive the Black-Scholes inputs and state the delta/gamma/vega formulas and
  units;
- derive risk-neutral GBM stepping and all three discounted payoffs;
- explain Welford add/merge, standard error, intervals, antithetic pairing, and
  the analytical control variate;
- trace `(seed, path, step)` through mixing, open uniforms, and Box-Muller;
- explain why 2,048-observation blocks and ordered merging give the supported
  reproducibility property;
- defend the profiling and benchmark protocol, including why scaling is
  non-linear;
- explain every CMake option, CI job, test layer, binding boundary, and
  sanitizer limitation;
- calculate every convergence metric from a small example; and
- state every limitation above before making a broader claim.

The evidence-backed résumé wording remains in `docs/evidence_log.md`. Do not
strengthen it to “production system,” “continuous barrier model,”
“cross-platform bitwise reproducibility,” or an unqualified speedup.
