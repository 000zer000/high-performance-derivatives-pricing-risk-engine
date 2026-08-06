# Benchmarking in continuous integration

The repository separates **correctness gates** from **performance evidence**.
Unit tests and numerical equality checks are deterministic requirements. Runtime
measurements on shared GitHub-hosted runners are not.

## What the benchmark-smoke job does

The `ubuntu-benchmark-smoke` job builds the Release benchmark target with
OpenMP, runs reduced European and Asian Monte Carlo workloads, stores the raw
`key=value` output, and validates it with
`benchmarks/check_benchmark_output.py`.

The validator checks that:

- both expected benchmark sections are present;
- serial and OpenMP numerical results are exactly equal;
- timing and speedup fields are finite and positive;
- the OpenMP run used at least two threads;
- neither median runtime exceeds a deliberately broad safety limit;
- parallel execution has not degraded below a deliberately broad speedup floor.

The raw output is uploaded as a workflow artifact for 14 days, including when
the validation step fails.

## Why the thresholds are broad

GitHub-hosted runners vary in hardware, co-tenancy, CPU frequency, thermal
state, and OpenMP runtime behaviour. Tight runtime or speedup thresholds would
create false failures and would not constitute a controlled performance
experiment.

The CI floor is therefore a **catastrophic-regression detector**, not a claim
that a particular speedup is reproduced on every runner. The committed Apple
M1 measurements remain the controlled, evidence-backed benchmark record.

## Reproduce the smoke check locally

```bash
cmake -S . -B build-benchmark \
  -DCMAKE_BUILD_TYPE=Release \
  -DDPR_WARNINGS_AS_ERRORS=ON \
  -DDPR_ENABLE_OPENMP=ON \
  -DDPR_BUILD_BENCHMARKS=ON
cmake --build build-benchmark --parallel --target pricing_benchmark

./build-benchmark/pricing_benchmark 200000 20000 16 3 2 \
  | tee benchmark-smoke.txt

python3 benchmarks/check_benchmark_output.py \
  benchmark-smoke.txt \
  --min-speedup 0.25 \
  --max-seconds 30 \
  --min-threads 2
```

A machine without an OpenMP-enabled compiler/runtime cannot execute this check.
That is an environment limitation, not a numerical failure.

## Interpreting failures

- **Missing or malformed fields:** benchmark output contract changed or the
  executable did not complete normally.
- **Numerical mismatch:** investigate immediately; serial/OpenMP equality is a
  correctness requirement.
- **Thread-count failure:** OpenMP was unavailable or the runtime did not honour
  the requested configuration.
- **Runtime or speedup failure:** first inspect the raw artifact and rerun. Only
  treat repeated failures on comparable runners as evidence of a regression.
