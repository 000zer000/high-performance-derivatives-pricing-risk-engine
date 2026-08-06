#!/usr/bin/env python3
"""Validate benchmark output with intentionally broad CI sanity thresholds.

This check is designed to catch broken benchmark execution, numerical mismatch,
invalid timing data, or catastrophic slowdowns. It is not a stable performance
comparison across GitHub-hosted runners.
"""

from __future__ import annotations

import argparse
import math
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class BenchmarkResult:
    name: str
    numerical_results_equal: bool
    serial_median_seconds: float
    parallel_median_seconds: float
    median_speedup: float
    threads_used: int


def parse_bool(value: str, field: str) -> bool:
    normalized = value.strip().lower()
    if normalized == "true":
        return True
    if normalized == "false":
        return False
    raise ValueError(f"{field} must be true or false, got {value!r}")


def parse_positive_float(value: str, field: str) -> float:
    parsed = float(value)
    if not math.isfinite(parsed) or parsed <= 0.0:
        raise ValueError(f"{field} must be finite and positive, got {value!r}")
    return parsed


def parse_positive_int(value: str, field: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise ValueError(f"{field} must be positive, got {value!r}")
    return parsed


def parse_results(text: str) -> list[BenchmarkResult]:
    sections: list[dict[str, str]] = []
    current: dict[str, str] | None = None

    for line_number, raw_line in enumerate(text.splitlines(), start=1):
        line = raw_line.strip()
        if not line:
            continue
        if "=" not in line:
            raise ValueError(f"line {line_number} is not key=value data: {raw_line!r}")
        key, value = line.split("=", 1)
        if key == "benchmark":
            current = {"benchmark": value}
            sections.append(current)
        elif current is not None:
            current[key] = value

    results: list[BenchmarkResult] = []
    required = {
        "benchmark",
        "numerical_results_equal",
        "serial_median_seconds",
        "parallel_median_seconds",
        "median_speedup",
        "threads_used",
    }
    for section in sections:
        missing = sorted(required.difference(section))
        if missing:
            raise ValueError(
                f"benchmark {section.get('benchmark', '<unknown>')!r} is missing: "
                + ", ".join(missing)
            )
        results.append(
            BenchmarkResult(
                name=section["benchmark"],
                numerical_results_equal=parse_bool(
                    section["numerical_results_equal"],
                    "numerical_results_equal",
                ),
                serial_median_seconds=parse_positive_float(
                    section["serial_median_seconds"],
                    "serial_median_seconds",
                ),
                parallel_median_seconds=parse_positive_float(
                    section["parallel_median_seconds"],
                    "parallel_median_seconds",
                ),
                median_speedup=parse_positive_float(
                    section["median_speedup"],
                    "median_speedup",
                ),
                threads_used=parse_positive_int(
                    section["threads_used"],
                    "threads_used",
                ),
            )
        )
    return results


def validate(
    results: list[BenchmarkResult],
    expected_names: set[str],
    min_speedup: float,
    max_seconds: float,
    min_threads: int,
) -> None:
    observed_names = {result.name for result in results}
    if observed_names != expected_names:
        raise ValueError(
            f"expected benchmark sections {sorted(expected_names)}, "
            f"observed {sorted(observed_names)}"
        )

    for result in results:
        if not result.numerical_results_equal:
            raise ValueError(f"{result.name}: serial and parallel results differ")
        if result.threads_used < min_threads:
            raise ValueError(
                f"{result.name}: expected at least {min_threads} threads, "
                f"observed {result.threads_used}"
            )
        if result.serial_median_seconds > max_seconds:
            raise ValueError(
                f"{result.name}: serial median {result.serial_median_seconds:.6f}s "
                f"exceeds broad CI limit {max_seconds:.6f}s"
            )
        if result.parallel_median_seconds > max_seconds:
            raise ValueError(
                f"{result.name}: parallel median {result.parallel_median_seconds:.6f}s "
                f"exceeds broad CI limit {max_seconds:.6f}s"
            )
        if result.median_speedup < min_speedup:
            raise ValueError(
                f"{result.name}: median speedup {result.median_speedup:.3f}x "
                f"is below broad CI floor {min_speedup:.3f}x"
            )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--min-speedup", type=float, default=0.25)
    parser.add_argument("--max-seconds", type=float, default=30.0)
    parser.add_argument("--min-threads", type=int, default=2)
    args = parser.parse_args()

    if args.min_speedup <= 0.0:
        parser.error("--min-speedup must be positive")
    if args.max_seconds <= 0.0:
        parser.error("--max-seconds must be positive")
    if args.min_threads <= 0:
        parser.error("--min-threads must be positive")

    results = parse_results(args.output.read_text(encoding="utf-8"))
    validate(
        results,
        expected_names={"european_plain", "asian_antithetic"},
        min_speedup=args.min_speedup,
        max_seconds=args.max_seconds,
        min_threads=args.min_threads,
    )

    for result in results:
        print(
            f"{result.name}: serial={result.serial_median_seconds:.6f}s, "
            f"parallel={result.parallel_median_seconds:.6f}s, "
            f"speedup={result.median_speedup:.3f}x, "
            f"threads={result.threads_used}"
        )
    print("benchmark output passed broad CI sanity checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
