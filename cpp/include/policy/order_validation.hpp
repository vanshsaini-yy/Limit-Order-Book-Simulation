#pragma once
#include "models/order.hpp"

enum class RejectionReason : uint8_t {
    None,                               // No rejection, order is valid
    NullOrder,                          // nullptr passed
    InvalidQuantity,                    // qty <= 0
    InvalidPrice,                       // priceTicks <= 0 for limit orders
    AddingMarketOrder,                  // market order shouldn't be added to the book
    AddingDuplicateOrder,               // trying to add an order that already exists
    AddingCancelledOrder,               // trying to add an order that is already cancelled
    AddingExecutedOrder,                // trying to add an order that is already executed
    OrderToBeRemovedDoesNotExist,       // trying to cancel an order that doesn't exist
    OrderToBeRemovedAlreadyCancelled,   // trying to cancel an order that is already cancelled
    OrderToBeRemovedAlreadyExecuted,    // trying to cancel an order that is already executed
    OrderBookInvariantViolation         // order book invariant violation
};

class OrderValidator {
    public:
        static RejectionReason validateBeforeAdding(const OrderPtr &order) {
            if (!order) {
                return RejectionReason::NullOrder;
            }
            
            if (order->getType() == OrderType::Market) {
                return RejectionReason::AddingMarketOrder;
            }

            if (order->getQty() <= 0) {
                return RejectionReason::InvalidQuantity;
            }
            
            if (order->getPriceTicks() <= 0) {
                return RejectionReason::InvalidPrice;
            }

            if (order->isCancelled()) {
                return RejectionReason::AddingCancelledOrder;
            }

            if (order->isExecuted()) {
                return RejectionReason::AddingExecutedOrder;
            }
            
            return RejectionReason::None;
        }

        static RejectionReason validateBeforeRemoving(const OrderPtr &order) {
            if (!order || order->isCancelled() || order->isExecuted()) {
                return RejectionReason::OrderBookInvariantViolation;
            }

            return RejectionReason::None;
        }
};