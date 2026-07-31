#include "derivatives/black_scholes.hpp"
#include "derivatives/greeks.hpp"
#include "derivatives/monte_carlo.hpp"
#include "derivatives/option.hpp"
#include "derivatives/pricing_result.hpp"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(derivatives_engine, module)
{
    module.doc() = "Python bindings for the C++ derivatives pricing core";

    py::enum_<derivatives::OptionType>(module, "OptionType")
        .value("CALL", derivatives::OptionType::Call)
        .value("PUT", derivatives::OptionType::Put);

    py::enum_<derivatives::VarianceReduction>(module, "VarianceReduction")
        .value("NONE", derivatives::VarianceReduction::None)
        .value("ANTITHETIC", derivatives::VarianceReduction::Antithetic)
        .value("CONTROL_VARIATE", derivatives::VarianceReduction::ControlVariate);

    py::enum_<derivatives::ExecutionPolicy>(module, "ExecutionPolicy")
        .value("SERIAL", derivatives::ExecutionPolicy::Serial)
        .value("OPENMP", derivatives::ExecutionPolicy::OpenMP);

    py::class_<derivatives::EuropeanOption>(module, "EuropeanOption")
        .def(py::init<>())
        .def_readwrite("spot", &derivatives::EuropeanOption::spot)
        .def_readwrite("strike", &derivatives::EuropeanOption::strike)
        .def_readwrite(
            "risk_free_rate",
            &derivatives::EuropeanOption::risk_free_rate
        )
        .def_readwrite("volatility", &derivatives::EuropeanOption::volatility)
        .def_readwrite(
            "time_to_maturity",
            &derivatives::EuropeanOption::time_to_maturity
        )
        .def_readwrite("type", &derivatives::EuropeanOption::type);

    py::class_<derivatives::AsianOption>(module, "AsianOption")
        .def(py::init<>())
        .def_readwrite("underlying", &derivatives::AsianOption::underlying)
        .def_readwrite(
            "monitoring_steps",
            &derivatives::AsianOption::monitoring_steps
        );

    py::class_<derivatives::DownAndOutOption>(module, "DownAndOutOption")
        .def(py::init<>())
        .def_readwrite("underlying", &derivatives::DownAndOutOption::underlying)
        .def_readwrite("barrier", &derivatives::DownAndOutOption::barrier)
        .def_readwrite(
            "monitoring_steps",
            &derivatives::DownAndOutOption::monitoring_steps
        );

    py::class_<derivatives::MonteCarloConfig>(module, "MonteCarloConfig")
        .def(py::init<>())
        .def_readwrite("paths", &derivatives::MonteCarloConfig::paths)
        .def_readwrite("seed", &derivatives::MonteCarloConfig::seed)
        .def_readwrite(
            "variance_reduction",
            &derivatives::MonteCarloConfig::variance_reduction
        )
        .def_readwrite("execution", &derivatives::MonteCarloConfig::execution)
        .def_readwrite("threads", &derivatives::MonteCarloConfig::threads);

    py::class_<derivatives::MonteCarloResult>(module, "MonteCarloResult")
        .def_readonly("price", &derivatives::MonteCarloResult::price)
        .def_readonly(
            "standard_error",
            &derivatives::MonteCarloResult::standard_error
        )
        .def_readonly(
            "confidence_interval_lower",
            &derivatives::MonteCarloResult::confidence_interval_lower
        )
        .def_readonly(
            "confidence_interval_upper",
            &derivatives::MonteCarloResult::confidence_interval_upper
        )
        .def_readonly("paths", &derivatives::MonteCarloResult::paths)
        .def_readonly("seed", &derivatives::MonteCarloResult::seed)
        .def_readonly(
            "effective_samples",
            &derivatives::MonteCarloResult::effective_samples
        )
        .def_readonly(
            "threads_used",
            &derivatives::MonteCarloResult::threads_used
        )
        .def_readonly(
            "control_variate_beta",
            &derivatives::MonteCarloResult::control_variate_beta
        );

    py::class_<derivatives::Greeks>(module, "Greeks")
        .def_readonly("delta", &derivatives::Greeks::delta)
        .def_readonly("gamma", &derivatives::Greeks::gamma)
        .def_readonly("vega", &derivatives::Greeks::vega);

    module.def("black_scholes_price", &derivatives::black_scholes_price);
    module.def("black_scholes_greeks", &derivatives::black_scholes_greeks);
    module.def(
        "price_european_monte_carlo",
        &derivatives::price_european_monte_carlo
    );
    module.def(
        "price_asian_monte_carlo",
        &derivatives::price_asian_monte_carlo
    );
    module.def(
        "price_down_and_out_monte_carlo",
        &derivatives::price_down_and_out_monte_carlo
    );
    module.def("openmp_available", &derivatives::openmp_available);
}
