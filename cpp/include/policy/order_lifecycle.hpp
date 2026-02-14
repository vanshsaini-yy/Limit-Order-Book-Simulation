#pragma once
#include "models/order.hpp"

class OrderLifecycle {
public:
    static OrderStatus afterCancelIncoming(const Quantity initialQty, const Quantity remainingQty) {
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
        } else {
            return currentStatus;
        }
    }

    static OrderStatus afterMatching(const Quantity initialQty, const Quantity remainingQty, const OrderType type) {
        if (type == OrderType::Cancel) {
            return OrderStatus::Executed;
        }

        if (remainingQty == 0) {
            return OrderStatus::Executed;
        } else if (remainingQty < initialQty) {
            return type == OrderType::Limit ? OrderStatus::PartiallyExecuted : OrderStatus::CancelledAfterPartialExecution;
        } else {
            return type == OrderType::Limit ? OrderStatus::Pending : OrderStatus::Cancelled;
        }
    }
};