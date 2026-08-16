#pragma once
#include "models/order.hpp"

enum class RejectionReason : uint8_t {
    None,                               // No rejection, order is valid
    NullOrder,                          // nullptr passed
    InvalidOrderType,                   // order type is not recognized
    InvalidLimitOrder,                  // limit order that doesn't meet the criteria for a valid limit order
    InvalidMarketOrder,                 // market order that doesn't meet the criteria for a valid market order
    InvalidCancelOrder,                 // cancel order that doesn't meet the criteria for a valid cancel order
    DuplicateOrderID,                   // trying to add or match an order whose ID already exists in the book
    OrderToBeCancelledDoesNotExist,     // trying to cancel an order that doesn't exist or trying to cancel an order that wasn't placed by the same owner
    OrderBookInvariantViolation,        // order book invariant violation
    FOKInsufficientLiquidity,           // FOK order could not be filled in full against the resting book
    InvalidPostOnlyOrder,               // post-only flag set on a non-GTC-limit order (e.g. Market, or combined with IOC/FOK)
    PostOnlyWouldCross,                 // post-only order would have matched immediately against the resting book
    PriceCollarViolation,               // limit order priced outside the allowed deviation from the reference price
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
