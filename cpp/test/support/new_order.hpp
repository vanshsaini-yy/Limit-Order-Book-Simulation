#pragma once
#include "models/new_order_request.hpp"

// The request ID is the timestamp, so it is unique within a test as long as timestamps are.
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
    return NewOrderRequest(static_cast<RequestID>(timestamp), ownerID, timestamp, orderID, priceTicks, qty, side, type, timeInForce, postOnly);
}
