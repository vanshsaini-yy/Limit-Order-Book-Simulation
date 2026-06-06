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
    OrderToBeCancelledDoesNotExist,     // trying to cancel an order that doesn't exist or trying to cancel an order that wasn't placed by the same owner
    OrderBookInvariantViolation,        // order book invariant violation
};

class OrderValidator {
    public:
        static RejectionReason validateLimitOrder(const OrderPtr &order, bool allowPartialExecution = false);
        static RejectionReason validateMarketOrder(const OrderPtr &order);
        static RejectionReason validateCancelOrder(const OrderPtr &order);
        static RejectionReason validateBeforeAdding(const OrderPtr &order);
        static RejectionReason validateBeforeCancelling(const OrderPtr &order);
        static RejectionReason validateBeforeMatching(const OrderPtr &order);
};
