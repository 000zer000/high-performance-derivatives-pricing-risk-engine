# Mathematics

This document will be completed alongside implementation. No formula is
currently claimed as implemented.

## Model assumptions to derive

- no dividends in v0.1.0;
- constant risk-free rate;
- constant volatility;
- geometric Brownian motion under the risk-neutral measure;
- European exercise;
- discounted payoff expectation.

## Required derivations

1. Black–Scholes European call and put prices.
2. Put-call parity.
3. Risk-neutral terminal stock distribution.
4. Monte Carlo estimator.
5. Sample variance and standard error.
6. Two-sided 95% confidence interval.

## Reference-case policy

Every hard-coded expected value must include:

- input parameters;
- units and conventions;
- independent calculation method;
- sufficient precision;
- test tolerance and justification.
