#pragma once
#include "models/order.hpp"

class OrderLifecycle {
public:
    static OrderStatus afterCancelIncoming(const Quantity initialQty, const Quantity remainingQty);
    static OrderStatus afterCancelResting(const OrderStatus currentStatus);
    static OrderStatus afterMatching(const Quantity initialQty, const Quantity remainingQty, const OrderType type);
};
