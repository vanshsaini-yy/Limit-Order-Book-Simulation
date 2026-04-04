#include <pybind11/pybind11.h>

#include "matching_engine_facade_bindings.hpp"
#include "order_bindings.hpp"

namespace py = pybind11;

PYBIND11_MODULE(limit_order_book, module) {
    module.doc() = "Python bindings for limit order book simulation";
    bindOrderTypes(module);
    bindMatchingEngineFacade(module);
}
