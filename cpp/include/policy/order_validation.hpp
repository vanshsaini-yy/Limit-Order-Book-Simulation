#pragma once
#include "models/order.hpp"

enum class RejectionReason : uint8_t {
    None,                               // No rejection, order is valid
    NullOrder,                          // nullptr passed
    InvalidOrderType,                   // order type is not recognized
    InvalidLimitOrder,                  // limit order that doesn't meet the criteria for a valid limit order
    InvalidMarketOrder,                 // market order that doesn't meet the criteria for a valid market order
    InvalidCancelOrder,                 // cancel order that doesn't meet the criteria for a valid cancel order
    OrderToBeAddedAlreadyExists,        // trying to add an order that already exists
    OrderToBeCancelledDoesNotExist,     // trying to cancel an order that doesn't exist
    OrderBookInvariantViolation,        // order book invariant violation
};

class OrderValidator {
    public:
        static RejectionReason validateLimitOrder(const OrderPtr &order, bool allowPartialExecution = false) {
            if (order->getPriceTicks() > 0 &&
                order->getQty() > 0 &&
                order->getSide() != Side::None &&
                (order->getStatus() == OrderStatus::Pending || (allowPartialExecution && order->getStatus() == OrderStatus::PartiallyExecuted)) &&
                order->getOrderID() != 0 &&
                order->getLinkedOrderID() == 0) {
                return RejectionReason::None;
            }

            return RejectionReason::InvalidLimitOrder;
        }

        static RejectionReason validateMarketOrder(const OrderPtr &order) {
            if (order->getPriceTicks() == 0 &&
                order->getQty() > 0 &&
                order->getSide() != Side::None &&
                order->getStatus() == OrderStatus::Pending &&
                order->getOrderID() != 0 &&
                order->getLinkedOrderID() == 0) {
                return RejectionReason::None;
            }

            return RejectionReason::InvalidMarketOrder;
        }

        static RejectionReason validateCancelOrder(const OrderPtr &order) {
            if (order->getPriceTicks() == 0 &&
                order->getQty() == 0 &&
                order->getSide() == Side::None &&
                order->getStatus() == OrderStatus::Pending &&
                order->getOrderID() != 0 &&
                order->getLinkedOrderID() != 0 && 
                order->getLinkedOrderID() != order->getOrderID()) {
                return RejectionReason::None;
            }

            return RejectionReason::InvalidCancelOrder;
        }

        static RejectionReason validateBeforeAdding(const OrderPtr &order) {
            if (order && validateLimitOrder(order, true) == RejectionReason::None) {
                return RejectionReason::None;
            }
            return RejectionReason::OrderBookInvariantViolation;
        }

        static RejectionReason validateBeforeCancelling(const OrderPtr &order) {
            if (order && validateLimitOrder(order, true) == RejectionReason::None) {
                return RejectionReason::None;
            }
            return RejectionReason::OrderBookInvariantViolation;
        }

        static RejectionReason validateBeforeMatching(const OrderPtr &order) {
            if (!order) {
                return RejectionReason::NullOrder;
            }

            if(order->getType() == OrderType::Limit) {
                return validateLimitOrder(order);
            }

            if (order->getType() == OrderType::Market) {
                return validateMarketOrder(order);
            }

            if (order->getType() == OrderType::Cancel) {
                return validateCancelOrder(order);
            }

            return RejectionReason::InvalidOrderType;
        }
};