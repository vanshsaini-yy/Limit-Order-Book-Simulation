#include "order_bindings.hpp"

#include "models/order.hpp"
#include "policy/order_validation.hpp"

namespace py = pybind11;

void bindOrderTypes(py::module_& module) {
    py::enum_<Side>(module, "Side")
        .value("Buy", Side::Buy)
        .value("Sell", Side::Sell)
        .value("None", Side::None)
        .export_values();

    py::enum_<OrderType>(module, "OrderType")
        .value("Limit", OrderType::Limit)
        .value("Market", OrderType::Market)
        .value("Cancel", OrderType::Cancel)
        .export_values();

    py::enum_<OrderStatus>(module, "OrderStatus")
        .value("Pending", OrderStatus::Pending)
        .value("PartiallyExecuted", OrderStatus::PartiallyExecuted)
        .value("Executed", OrderStatus::Executed)
        .value("Cancelled", OrderStatus::Cancelled)
        .value("CancelledAfterPartialExecution", OrderStatus::CancelledAfterPartialExecution)
        .export_values();

    py::enum_<RejectionReason>(module, "RejectionReason")
        .value("None", RejectionReason::None)
        .value("NullOrder", RejectionReason::NullOrder)
        .value("InvalidOrderType", RejectionReason::InvalidOrderType)
        .value("InvalidLimitOrder", RejectionReason::InvalidLimitOrder)
        .value("InvalidMarketOrder", RejectionReason::InvalidMarketOrder)
        .value("InvalidCancelOrder", RejectionReason::InvalidCancelOrder)
        .value("OrderToBeAddedAlreadyExists", RejectionReason::OrderToBeAddedAlreadyExists)
        .value("OrderToBeCancelledDoesNotExist", RejectionReason::OrderToBeCancelledDoesNotExist)
        .value("OrderBookInvariantViolation", RejectionReason::OrderBookInvariantViolation)
        .export_values();

    py::class_<Order, std::shared_ptr<Order>>(module, "Order")
        .def(
            py::init<OrderID, OwnerID, PriceTicks, Quantity, Side, OrderType, Timestamp, OrderID>(),
            py::arg("order_id"),
            py::arg("owner_id"),
            py::arg("price_ticks"),
            py::arg("qty"),
            py::arg("side"),
            py::arg("type"),
            py::arg("timestamp"),
            py::arg("linked_order_id") = 0
        )
        .def_property_readonly("order_id", &Order::getOrderID)
        .def_property_readonly("owner_id", &Order::getOwnerID)
        .def_property_readonly("price_ticks", &Order::getPriceTicks)
        .def_property_readonly("qty", &Order::getQty)
        .def_property_readonly("side", &Order::getSide)
        .def_property_readonly("type", &Order::getType)
        .def_property_readonly("timestamp", &Order::getTimestamp)
        .def_property_readonly("status", &Order::getStatus)
        .def_property_readonly("linked_order_id", &Order::getLinkedOrderID)
        .def("is_cancelled", &Order::isCancelled)
        .def("is_executed", &Order::isExecuted);
}
