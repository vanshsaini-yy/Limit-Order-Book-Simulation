#include "order_bindings.hpp"

#include "models/order.hpp"
#include "models/rejection_reason.hpp"

namespace py = pybind11;

void bindOrderTypes(py::module_& module) {
    py::enum_<Side>(module, "Side")
        .value("BUY", Side::Buy)
        .value("SELL", Side::Sell)
        .value("NONE", Side::None);

    py::enum_<OrderType>(module, "OrderType")
        .value("LIMIT", OrderType::Limit)
        .value("MARKET", OrderType::Market)
        .value("CANCEL", OrderType::Cancel);

    py::enum_<TimeInForce>(module, "TimeInForce")
        .value("GTC", TimeInForce::GTC)
        .value("IOC", TimeInForce::IOC)
        .value("FOK", TimeInForce::FOK);

    py::enum_<OrderStatus>(module, "OrderStatus")
        .value("PENDING", OrderStatus::Pending)
        .value("PARTIALLY_EXECUTED", OrderStatus::PartiallyExecuted)
        .value("EXECUTED", OrderStatus::Executed)
        .value("CANCELLED", OrderStatus::Cancelled)
        .value("CANCELLED_AFTER_PARTIAL_EXECUTION", OrderStatus::CancelledAfterPartialExecution);

    py::enum_<RejectionReason>(module, "RejectionReason")
        .value("NONE", RejectionReason::None)
        .value("NULL_ORDER", RejectionReason::NullOrder)
        .value("INVALID_ORDER_TYPE", RejectionReason::InvalidOrderType)
        .value("INVALID_LIMIT_ORDER", RejectionReason::InvalidLimitOrder)
        .value("INVALID_MARKET_ORDER", RejectionReason::InvalidMarketOrder)
        .value("INVALID_CANCEL_ORDER", RejectionReason::InvalidCancelOrder)
        .value("DUPLICATE_ORDER_ID", RejectionReason::DuplicateOrderID)
        .value("ORDER_TO_BE_CANCELLED_DOES_NOT_EXIST", RejectionReason::OrderToBeCancelledDoesNotExist)
        .value("ORDER_BOOK_INVARIANT_VIOLATION", RejectionReason::OrderBookInvariantViolation)
        .value("FOK_INSUFFICIENT_LIQUIDITY", RejectionReason::FOKInsufficientLiquidity)
        .value("INVALID_POST_ONLY_ORDER", RejectionReason::InvalidPostOnlyOrder)
        .value("POST_ONLY_WOULD_CROSS", RejectionReason::PostOnlyWouldCross)
        .value("PRICE_COLLAR_VIOLATION", RejectionReason::PriceCollarViolation);

    py::class_<Order, std::shared_ptr<Order>>(module, "Order")
        .def(
            py::init<OrderID, OwnerID, PriceTicks, Quantity, Side, OrderType, Timestamp, OrderID, TimeInForce, bool>(),
            py::arg("order_id"),
            py::arg("owner_id"),
            py::arg("price_ticks"),
            py::arg("qty"),
            py::arg("side"),
            py::arg("order_type"),
            py::arg("timestamp"),
            py::arg("linked_order_id") = 0,
            py::arg("time_in_force") = TimeInForce::GTC,
            py::arg("post_only") = false
        )
        .def_property_readonly("order_id", &Order::getOrderID)
        .def_property_readonly("owner_id", &Order::getOwnerID)
        .def_property_readonly("price_ticks", &Order::getPriceTicks)
        .def_property_readonly("qty", &Order::getQty)
        .def_property_readonly("side", &Order::getSide)
        .def_property_readonly("order_type", &Order::getType)
        .def_property_readonly("timestamp", &Order::getTimestamp)
        .def_property_readonly("status", &Order::getStatus)
        .def_property_readonly("linked_order_id", &Order::getLinkedOrderID)
        .def_property_readonly("time_in_force", &Order::getTimeInForce)
        .def_property_readonly("post_only", &Order::isPostOnly)
        .def("is_cancelled", &Order::isCancelled)
        .def("is_executed", &Order::isExecuted);
}
