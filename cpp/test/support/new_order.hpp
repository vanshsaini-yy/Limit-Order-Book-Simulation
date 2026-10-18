#pragma once
#include "models/new_order_request.hpp"

inline NewOrderRequest newOrderRequestHelper(
    OrderID orderID,
    OwnerID ownerID,
    PriceTicks priceTicks,
    Quantity qty,
    Side side,
    OrderType type,
    Timestamp timestamp,
    TimeInForce timeInForce = TimeInForce::GTC,
    bool postOnly = false
) {
    return NewOrderRequest(orderID, ownerID, timestamp, orderID, priceTicks, qty, side, type, timeInForce, postOnly);
}
