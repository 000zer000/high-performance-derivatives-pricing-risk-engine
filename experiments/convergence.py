#!/usr/bin/env python3
"""Run a reproducible multi-seed European Monte Carlo convergence study."""

from __future__ import annotations

import argparse
import math
import statistics
import subprocess
from pathlib import Path


SPOT = 100.0
STRIKE = 100.0
RATE = 0.05
VOLATILITY = 0.20
MATURITY = 1.0


def analytical_call() -> float:
    root_time = math.sqrt(MATURITY)
    d1 = (
        math.log(SPOT / STRIKE)
        + (RATE + 0.5 * VOLATILITY * VOLATILITY) * MATURITY
    ) / (VOLATILITY * root_time)
    d2 = d1 - VOLATILITY * root_time
    normal_cdf = lambda value: 0.5 * (
        1.0 + math.erf(value / math.sqrt(2.0))
    )
    return SPOT * normal_cdf(d1) - STRIKE * math.exp(
        -RATE * MATURITY
    ) * normal_cdf(d2)


def parse_key_values(output: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for line in output.splitlines():
        if "=" in line:
            key, value = line.split("=", maxsplit=1)
            fields[key] = value
    return fields


def one_run(cli: Path, paths: int, seed: int, method: str) -> tuple[float, float, float, float]:
    command = [
        str(cli),
        "monte-carlo",
        "european",
        "call",
        "--spot",
        str(SPOT),
        "--strike",
        str(STRIKE),
        "--rate",
        str(RATE),
        "--volatility",
        str(VOLATILITY),
        "--maturity",
        str(MATURITY),
        "--paths",
        str(paths),
        "--seed",
        str(seed),
        "--variance-reduction",
        method,
    ]
    completed = subprocess.run(
        command,
        check=True,
        capture_output=True,
        text=True,
    )
    fields = parse_key_values(completed.stdout)
    return (
        float(fields["price"]),
        float(fields["standard_error"]),
        float(fields["ci95_lower"]),
        float(fields["ci95_upper"]),
    )


def summarize(cli: Path, paths: int, trials: int, method: str) -> str:
    reference = analytical_call()
    runs = [
        one_run(cli, paths, 20_260_731 + trial, method)
        for trial in range(trials)
    ]
    estimates = [run[0] for run in runs]
    errors = [estimate - reference for estimate in estimates]
    mean_estimate = statistics.fmean(estimates)
    bias = mean_estimate - reference
    rmse = math.sqrt(statistics.fmean(error * error for error in errors))
    empirical_standard_deviation = statistics.stdev(estimates)
    mean_reported_standard_error = statistics.fmean(run[1] for run in runs)
    coverage = statistics.fmean(
        1.0 if lower <= reference <= upper else 0.0
        for _, _, lower, upper in runs
    )
    return ",".join(
        [
            method,
            str(paths),
            str(trials),
            f"{reference:.17g}",
            f"{mean_estimate:.17g}",
            f"{bias:.17g}",
            f"{rmse:.17g}",
            f"{empirical_standard_deviation:.17g}",
            f"{mean_reported_standard_error:.17g}",
            f"{coverage:.17g}",
        ]
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True, type=Path)
    parser.add_argument("--paths", default="2000,20000,200000,2000000")
    parser.add_argument("--trials", default=10, type=int)
    arguments = parser.parse_args()
    path_counts = [int(value) for value in arguments.paths.split(",")]
    if arguments.trials < 2 or any(value < 4 or value % 2 for value in path_counts):
        raise ValueError("trials must be >= 2 and path counts must be even >= 4")

    print(
        "method,paths,trials,analytical,mean_estimate,bias,rmse,"
        "empirical_standard_deviation,mean_reported_standard_error,ci95_coverage"
    )
    for paths in path_counts:
        for method in ("none", "antithetic", "control"):
            print(summarize(arguments.cli, paths, arguments.trials, method))


if __name__ == "__main__":
    main()
