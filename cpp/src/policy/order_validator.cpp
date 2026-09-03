#include "policy/order_validator.hpp"

RejectionReason OrderValidator::validateLimitOrder(const Order& order, bool allowPartialExecution) {
    bool statusOk = order.getStatus() == OrderStatus::Pending ||
                    (allowPartialExecution && order.getStatus() == OrderStatus::PartiallyExecuted);
    if (order.getPriceTicks() > 0 &&
        order.getQty() > 0 &&
        order.getSide() != Side::None &&
        statusOk &&
        order.getOrderID() != 0 &&
        order.getLinkedOrderID() == 0) {
        if (order.isPostOnly() && order.getTimeInForce() != TimeInForce::GTC) {
            return RejectionReason::InvalidPostOnlyOrder;
        }
        return RejectionReason::None;
    }
    return RejectionReason::InvalidLimitOrder;
}

RejectionReason OrderValidator::validateMarketOrder(const Order& order) {
    if (order.getPriceTicks() == 0 &&
        order.getQty() > 0 &&
        order.getSide() != Side::None &&
        order.getStatus() == OrderStatus::Pending &&
        order.getOrderID() != 0 &&
        order.getLinkedOrderID() == 0 &&
        !order.isPostOnly()) {
        return RejectionReason::None;
    }
    return RejectionReason::InvalidMarketOrder;
}

RejectionReason OrderValidator::validateCancelOrder(const Order& order) {
    if (order.getPriceTicks() == 0 &&
        order.getQty() == 0 &&
        order.getSide() == Side::None &&
        order.getStatus() == OrderStatus::Pending &&
        order.getOrderID() != 0 &&
        order.getLinkedOrderID() != 0 &&
        order.getLinkedOrderID() != order.getOrderID() &&
        order.getTimeInForce() == TimeInForce::GTC &&
        !order.isPostOnly()) {
        return RejectionReason::None;
    }
    return RejectionReason::InvalidCancelOrder;
}

RejectionReason OrderValidator::validateBeforeAddingOrRemoving(const OrderPtr &order) {
    if (order && validateLimitOrder(*order, true) == RejectionReason::None) {
        return RejectionReason::None;
    }
    return RejectionReason::OrderBookInvariantViolation;
}

RejectionReason OrderValidator::validateBeforeMatching(const OrderPtr &order) {
    if (!order) {
        return RejectionReason::NullOrder;
    }
    if (order->getType() == OrderType::Limit) {
        return validateLimitOrder(*order);
    }
    if (order->getType() == OrderType::Market) {
        return validateMarketOrder(*order);
    }
    if (order->getType() == OrderType::Cancel) {
        return validateCancelOrder(*order);
    }
    return RejectionReason::InvalidOrderType;
}
