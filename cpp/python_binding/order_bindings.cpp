#include "order_bindings.hpp"

#include <pybind11/stl.h>

#include "engine/submit_result.hpp"
#include "models/order.hpp"
#include "models/rejection_reason.hpp"
#include "models/request.hpp"
#include "models/new_order_request.hpp"
#include "models/cancel_request.hpp"
#include "models/modify_request.hpp"

namespace py = pybind11;

// TODO: this file is accumulating too much
void bindOrderTypes(py::module_& module) {
    py::enum_<Side>(module, "Side")
        .value("BUY", Side::Buy)
        .value("SELL", Side::Sell);

    py::enum_<OrderType>(module, "OrderType")
        .value("LIMIT", OrderType::Limit)
        .value("MARKET", OrderType::Market);

    py::enum_<RequestType>(module, "RequestType")
        .value("NEW", RequestType::New)
        .value("CANCEL", RequestType::Cancel)
        .value("MODIFY", RequestType::Modify);

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
        .value("SELF_TRADE_PREVENTION", RejectionReason::SelfTradePrevention)
        .value("FOK_INSUFFICIENT_LIQUIDITY", RejectionReason::FOKInsufficientLiquidity)
        .value("INVALID_POST_ONLY_ORDER", RejectionReason::InvalidPostOnlyOrder)
        .value("POST_ONLY_WOULD_CROSS", RejectionReason::PostOnlyWouldCross)
        .value("PRICE_COLLAR_VIOLATION", RejectionReason::PriceCollarViolation)
        .value("INVALID_MODIFY_ORDER", RejectionReason::InvalidModifyOrder)
        .value("ORDER_TO_BE_MODIFIED_DOES_NOT_EXIST", RejectionReason::OrderToBeModifiedDoesNotExist)
        .value("MODIFY_QUANTITY_BELOW_FILLED", RejectionReason::ModifyQuantityBelowFilled)
        .value("NO_OP_MODIFY", RejectionReason::NoOpModify);

    py::class_<Order, std::shared_ptr<Order>>(module, "Order")
        .def(
            py::init<OrderID, OwnerID, PriceTicks, Quantity, Side, OrderType, Timestamp, TimeInForce, bool>(),
            py::arg("order_id"),
            py::arg("owner_id"),
            py::arg("price_ticks"),
            py::arg("qty"),
            py::arg("side"),
            py::arg("order_type"),
            py::arg("timestamp"),
            py::arg("time_in_force") = TimeInForce::GTC,
            py::arg("post_only") = false
        )
        .def_property_readonly("order_id", &Order::getOrderID)
        .def_property_readonly("owner_id", &Order::getOwnerID)
        .def_property_readonly("price_ticks", &Order::getPriceTicks)
        .def_property_readonly("qty", &Order::getQty)
        .def_property_readonly("original_qty", &Order::getOriginalQty)
        .def_property_readonly("filled_qty", &Order::getFilledQty)
        .def_property_readonly("side", &Order::getSide)
        .def_property_readonly("order_type", &Order::getType)
        .def_property_readonly("timestamp", &Order::getTimestamp)
        .def_property_readonly("status", &Order::getStatus)
        .def_property_readonly("time_in_force", &Order::getTimeInForce)
        .def_property_readonly("post_only", &Order::isPostOnly)
        .def("is_cancelled", &Order::isCancelled)
        .def("is_executed", &Order::isExecuted);

    py::class_<IRequest>(module, "IRequest")
        .def_property_readonly("request_id", &IRequest::getRequestId)
        .def_property_readonly("request_type", &IRequest::getRequestType)
        .def_property_readonly("owner_id", &IRequest::getOwnerID)
        .def_property_readonly("timestamp", &IRequest::getTimestamp)
        .def("validate", &IRequest::validate);

    py::class_<NewOrderRequest, IRequest>(module, "NewOrderRequest")
        .def(
            py::init<RequestID, OwnerID, Timestamp, OrderID, PriceTicks, Quantity, Side, OrderType, TimeInForce, bool>(),
            py::arg("request_id"),
            py::arg("owner_id"),
            py::arg("timestamp"),
            py::arg("order_id"),
            py::arg("price_ticks"),
            py::arg("qty"),
            py::arg("side"),
            py::arg("order_type"),
            py::arg("time_in_force") = TimeInForce::GTC,
            py::arg("post_only") = false
        )
        .def_property_readonly("order_id", &NewOrderRequest::getOrderID)
        .def_property_readonly("price_ticks", &NewOrderRequest::getPriceTicks)
        .def_property_readonly("qty", &NewOrderRequest::getQty)
        .def_property_readonly("side", &NewOrderRequest::getSide)
        .def_property_readonly("order_type", &NewOrderRequest::getType)
        .def_property_readonly("time_in_force", &NewOrderRequest::getTimeInForce)
        .def_property_readonly("post_only", &NewOrderRequest::isPostOnly);

    py::class_<CancelRequest, IRequest>(module, "CancelRequest")
        .def(
            py::init<RequestID, OwnerID, Timestamp, OrderID>(),
            py::arg("request_id"),
            py::arg("owner_id"),
            py::arg("timestamp"),
            py::arg("target_order_id")
        )
        .def_property_readonly("target_order_id", &CancelRequest::getTargetOrderID);

    py::class_<ModifyRequest, IRequest>(module, "ModifyRequest")
        .def(
            py::init<RequestID, OwnerID, Timestamp, OrderID, std::optional<PriceTicks>, std::optional<Quantity>>(),
            py::arg("request_id"),
            py::arg("owner_id"),
            py::arg("timestamp"),
            py::arg("target_order_id"),
            py::arg("new_price_ticks") = std::nullopt,
            py::arg("new_original_qty") = std::nullopt
        )
        .def_property_readonly("target_order_id", &ModifyRequest::getTargetOrderID)
        .def_property_readonly("new_price_ticks", &ModifyRequest::getNewPriceTicks)
        .def_property_readonly("new_original_qty", &ModifyRequest::getNewOriginalQty);

    py::class_<SubmitResult>(module, "SubmitResult")
        .def_readonly("reason", &SubmitResult::reason)
        .def_readonly("order", &SubmitResult::order);
}
