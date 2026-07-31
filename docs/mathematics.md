# Mathematics

## Assumptions

The model uses geometric Brownian motion under the risk-neutral measure with
constant continuously compounded rate `r`, constant volatility `sigma`, no
dividends, and `double` arithmetic. `S > 0`, `K > 0`, `sigma >= 0`, and
`T >= 0`; finite negative rates are allowed.

## Black–Scholes price

For positive `T` and `sigma`:

```text
d1 = [ln(S / K) + (r + 0.5 sigma^2) T] / [sigma sqrt(T)]
d2 = d1 - sigma sqrt(T)

call = S N(d1) - K exp(-rT) N(d2)
put  = K exp(-rT) N(-d2) - S N(-d1)
```

`N(x) = 0.5 erfc(-x / sqrt(2))`. The code evaluates `ln(S) - ln(K)` to avoid
an unnecessary intermediate ratio. Expiry returns intrinsic value. At zero
volatility, the discounted deterministic payoff is returned.

Reference case `S=K=100`, `r=0.05`, `sigma=0.20`, `T=1`:

- call: `10.450583572185565`;
- put: `5.5735260222569707` from the independent Python reference;
- put-call parity residual: `0` in that reference.

## Analytical Greeks

For positive `T` and `sigma`, with standard-normal density `phi`:

```text
call delta = N(d1)
put delta  = N(d1) - 1
gamma      = phi(d1) / [S sigma sqrt(T)]
vega       = S phi(d1) sqrt(T)
```

Vega is per unit volatility change, not per one percentage point. The public
function rejects zero maturity or zero volatility because gamma/delta can be
non-smooth at those boundaries. Tests use independent frozen values, call/put
identities, and central finite differences of the independently implemented
price function.

## Monte Carlo paths

For a time increment `dt` and standard-normal draw `Z`:

```text
S(t + dt) = S(t) exp[(r - 0.5 sigma^2) dt + sigma sqrt(dt) Z]
```

European pricing needs one terminal step. The arithmetic Asian average is:

```text
A = (1 / m) sum[j=1..m] S(j T / m)
```

The initial spot is not included. The down-and-out contract checks the initial
spot and every one of `m` equally spaced future observations. If any monitored
spot is less than or equal to the barrier, payoff is zero; rebate is zero.

Every payoff is discounted by `exp(-rT)`. Discrete barrier monitoring creates
a different product from a continuously monitored barrier and introduces
monitoring bias if used as an approximation to the latter.

## Sampling uncertainty

Welford accumulators compute one-pass mean and sample variance with denominator
`n-1`:

```text
SE = sample_standard_deviation / sqrt(n)
CI95 = estimate +/- 1.96 SE
```

For antithetic sampling, `n` is the number of independent pair averages, not
the number of underlying paths. The interval describes Monte Carlo sampling
uncertainty under a normal approximation. It excludes model, parameter, and
time-discretization uncertainty.

## Antithetic variates

For every vector of normal draws `Z`, a second path uses `-Z`. One observation
is the average of the two discounted payoffs:

```text
Y_pair = [Y(Z) + Y(-Z)] / 2
```

Tests and measurements compare the pair-average standard error against plain
sampling for a frozen European call case. Variance reduction is case-dependent
and is not guaranteed for every payoff and parameter set.

## European stock control variate

The control is discounted terminal stock:

```text
X = exp(-rT) S_T
E[X] = S
Var[X] = S^2 [exp(sigma^2 T) - 1]
Y_cv = Y - beta (X - S)
beta = Cov(Y, X) / Var(X)
```

The implementation uses closed-form truncated lognormal moments for beta. Let
`v = sigma sqrt(T)` and `P` be the Black–Scholes option price.

For a call:

```text
E[YX] = S^2 exp(v^2) N(d1 + v) - K S exp(-rT) N(d1)
```

For a put:

```text
E[YX] = K S exp(-rT) N(-d1) - S^2 exp(v^2) N(-d1 - v)
```

Then `Cov(Y,X) = E[YX] - P S`. Because beta is analytical rather than fitted
on the pricing sample, the adjusted observations can use the ordinary sample
variance formula. The method is intentionally rejected for Asian and barrier
contracts; no unsupported beta is invented.

## Convergence experiment

[`experiments/convergence.py`](../experiments/convergence.py) independently
computes the Black–Scholes reference with Python `math.erf`, runs 30 fixed seeds
for each path count and estimator, and reports bias, RMSE, empirical standard
deviation, mean reported SE, and interval coverage. The committed CSV is a
finite numerical experiment, not a formal proof of convergence rate or exact
95% coverage.
