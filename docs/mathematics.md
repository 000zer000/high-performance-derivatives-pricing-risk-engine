# Mathematics

## Model assumptions

v0.1.0 assumes:

- no dividends;
- constant continuously compounded risk-free rate `r`;
- constant volatility `sigma`;
- geometric Brownian motion under the risk-neutral measure;
- European exercise at maturity `T`;
- monetary calculations represented by `double`.

`S` denotes spot and `K` strike. Spot and strike must be positive. Volatility
and maturity must be non-negative. A finite negative rate is allowed.

## Black–Scholes price

For positive `T` and `sigma`:

```text
d1 = [ln(S / K) + (r + 0.5 sigma^2) T] / [sigma sqrt(T)]
d2 = d1 - sigma sqrt(T)

call = S N(d1) - K exp(-rT) N(d2)
put  = K exp(-rT) N(-d2) - S N(-d1)
```

`N` is the standard-normal cumulative distribution function, implemented as:

```text
N(x) = 0.5 erfc(-x / sqrt(2))
```

The implementation evaluates `ln(S) - ln(K)` instead of `ln(S / K)` to avoid
an avoidable intermediate ratio overflow or underflow.

At `T = 0`, price is intrinsic value. At `sigma = 0` and positive maturity,
the discounted deterministic prices are:

```text
call = max(S - K exp(-rT), 0)
put  = max(K exp(-rT) - S, 0)
```

The implementation also tests put–call parity:

```text
call - put = S - K exp(-rT)
```

## Risk-neutral terminal simulation

For an independent standard-normal draw `Z_i`:

```text
S_T(i) = S exp[(r - 0.5 sigma^2) T + sigma sqrt(T) Z_i]
```

The discounted payoff sample is:

```text
Y_i = exp(-rT) max(S_T(i) - K, 0)  for a call
Y_i = exp(-rT) max(K - S_T(i), 0)  for a put
```

With `n` paths, the Monte Carlo price is the sample mean:

```text
price = (1 / n) sum(Y_i)
```

## Sample uncertainty

Welford’s algorithm accumulates the sample mean and squared deviations in one
pass. Sample variance uses Bessel’s correction:

```text
s^2 = sum[(Y_i - mean)^2] / (n - 1)
SE  = s / sqrt(n)
```

The reported two-sided normal-approximation interval is:

```text
[price - 1.96 SE, price + 1.96 SE]
```

A correct estimator is not required to contain the analytical reference in
every 95% interval. Fixed-seed tests instead require the observed error to be
within four reported standard errors as a broad correctness screen.

## Independent analytical reference

Reference inputs:

- `S = 100`;
- `K = 100`;
- `r = 0.05`;
- `sigma = 0.20`;
- `T = 1`.

An independent Python standard-library implementation produced:

- `d1 = 0.35000000000000003`;
- `d2 = 0.15000000000000002`;
- call `10.450583572185565`;
- put `5.5735260222569707`;
- put–call parity residual `0`.

Recomputation command:

```bash
python3 -c 'import math; s=100.; k=100.; r=.05; v=.20; t=1.; n=lambda x:.5*(1+math.erf(x/math.sqrt(2))); d1=(math.log(s/k)+(r+.5*v*v)*t)/(v*math.sqrt(t)); d2=d1-v*math.sqrt(t); c=s*n(d1)-k*math.exp(-r*t)*n(d2); p=k*math.exp(-r*t)*n(-d2)-s*n(-d1); print(d1,d2,c,p,c-p-(s-k*math.exp(-r*t)))'
```

The C++ tests use absolute tolerance `1e-10` for these independently recorded
prices and `1e-12` for put–call parity.

## Fixed-seed numerical observation

For 250,000 paths and seed 20,260,731 on the audited Release toolchain:

| Type | Analytical | Monte Carlo | Absolute error | SE | Error / SE |
|---|---:|---:|---:|---:|---:|
| Call | 10.450583572185565 | 10.405958339551766 | 0.04462523263379836 | 0.029309857309524682 | 1.5225332612 |
| Put | 5.5735260222569734 | 5.5710527461979824 | 0.0024732760589909475 | 0.017306743952028705 | 0.1429082250 |

This is one predeclared seed and one parameter case. It validates plumbing and
the broad four-standard-error screen; it is not a convergence study or a
general accuracy claim.
