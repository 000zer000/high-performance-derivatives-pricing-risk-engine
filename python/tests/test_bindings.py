import math
import unittest

import derivatives_engine as engine


def european_option() -> engine.EuropeanOption:
    option = engine.EuropeanOption()
    option.spot = 100.0
    option.strike = 100.0
    option.risk_free_rate = 0.05
    option.volatility = 0.20
    option.time_to_maturity = 1.0
    option.type = engine.OptionType.CALL
    return option


def monte_carlo_config() -> engine.MonteCarloConfig:
    config = engine.MonteCarloConfig()
    config.paths = 20_000
    config.seed = 20_260_731
    return config


class BindingTests(unittest.TestCase):
    def test_black_scholes_reference(self) -> None:
        self.assertAlmostEqual(
            engine.black_scholes_price(european_option()),
            10.450583572185565,
            places=12,
        )

    def test_greeks_reference(self) -> None:
        greeks = engine.black_scholes_greeks(european_option())
        self.assertAlmostEqual(greeks.delta, 0.6368306511756191, places=12)
        self.assertAlmostEqual(greeks.gamma, 0.018762017345846895, places=12)
        self.assertAlmostEqual(greeks.vega, 37.52403469169379, places=11)

    def test_monte_carlo_repeats_exactly(self) -> None:
        option = european_option()
        config = monte_carlo_config()
        first = engine.price_european_monte_carlo(option, config)
        second = engine.price_european_monte_carlo(option, config)
        self.assertEqual(first.price, second.price)
        self.assertEqual(first.standard_error, second.standard_error)
        self.assertEqual(first.paths, 20_000)
        self.assertEqual(first.seed, 20_260_731)

    def test_control_variate_has_smaller_error_bar(self) -> None:
        option = european_option()
        plain_config = monte_carlo_config()
        controlled_config = monte_carlo_config()
        controlled_config.variance_reduction = (
            engine.VarianceReduction.CONTROL_VARIATE
        )
        plain = engine.price_european_monte_carlo(option, plain_config)
        controlled = engine.price_european_monte_carlo(
            option, controlled_config
        )
        self.assertLess(controlled.standard_error, plain.standard_error)
        self.assertGreater(controlled.control_variate_beta, 0.0)

    def test_asian_and_barrier_contracts(self) -> None:
        asian = engine.AsianOption()
        asian.underlying = european_option()
        asian.monitoring_steps = 12
        asian_result = engine.price_asian_monte_carlo(
            asian, monte_carlo_config()
        )
        self.assertTrue(math.isfinite(asian_result.price))
        self.assertGreater(asian_result.price, 0.0)

        barrier = engine.DownAndOutOption()
        barrier.underlying = european_option()
        barrier.barrier = 100.0
        barrier.monitoring_steps = 12
        barrier_result = engine.price_down_and_out_monte_carlo(
            barrier, monte_carlo_config()
        )
        self.assertEqual(barrier_result.price, 0.0)

    def test_invalid_input_becomes_value_error(self) -> None:
        option = european_option()
        option.spot = 0.0
        with self.assertRaises(ValueError):
            engine.black_scholes_price(option)

    def test_openmp_matches_serial(self) -> None:
        if not engine.openmp_available():
            self.skipTest("OpenMP was not compiled into this build")
        option = european_option()
        serial_config = monte_carlo_config()
        parallel_config = monte_carlo_config()
        parallel_config.execution = engine.ExecutionPolicy.OPENMP
        parallel_config.threads = 2
        serial = engine.price_european_monte_carlo(option, serial_config)
        parallel = engine.price_european_monte_carlo(option, parallel_config)
        self.assertEqual(serial.price, parallel.price)
        self.assertEqual(serial.standard_error, parallel.standard_error)
        self.assertEqual(parallel.threads_used, 2)


if __name__ == "__main__":
    unittest.main()
