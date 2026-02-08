#pragma once
#include "models/order.hpp"

class OrderLifecycle {
public:
    static OrderStatus afterCancelIncoming(const uint32_t initialQty, const uint32_t remainingQty) {
        if (remainingQty < initialQty) {
            return OrderStatus::CancelledAfterPartialExecution;
        } else {
            return OrderStatus::Cancelled;
        }
    }

    static OrderStatus afterCancelResting(const OrderStatus currentStatus) {
        if (currentStatus == OrderStatus::Pending) {
            return OrderStatus::Cancelled;
        } else if (currentStatus == OrderStatus::PartiallyExecuted) {
            return OrderStatus::CancelledAfterPartialExecution;
        }
    }

    static OrderStatus afterMatching(const uint32_t initialQty, const uint32_t remainingQty, const OrderType type) {
        if (remainingQty == 0) {
            return OrderStatus::Executed;
        } else if (remainingQty < initialQty) {
            return type == OrderType::Limit ? OrderStatus::PartiallyExecuted : OrderStatus::CancelledAfterPartialExecution;
        } else {
            return type == OrderType::Limit ? OrderStatus::Pending : OrderStatus::Cancelled;
        }
    }
};