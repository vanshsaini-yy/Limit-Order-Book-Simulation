#include "models/new_order_request.hpp"

NewOrderRequest::NewOrderRequest(
    RequestID requestId_,
    OwnerID ownerID_,
    Timestamp timestamp_,
    OrderID orderID_,
    PriceTicks priceTicks_,
    Quantity qty_,
    Side side_,
    OrderType type_,
    TimeInForce timeInForce_,
    bool postOnly_
)
:   IRequest(requestId_, RequestType::New, ownerID_, timestamp_),
    orderID(orderID_),
    priceTicks(priceTicks_),
    qty(qty_),
    side(side_),
    type(type_),
    timeInForce(timeInForce_),
    postOnly(postOnly_) {}

OrderID     NewOrderRequest::getOrderID()     const { return orderID; }
PriceTicks  NewOrderRequest::getPriceTicks()  const { return priceTicks; }
Quantity    NewOrderRequest::getQty()         const { return qty; }
Side        NewOrderRequest::getSide()        const { return side; }
OrderType   NewOrderRequest::getType()        const { return type; }
TimeInForce NewOrderRequest::getTimeInForce() const { return timeInForce; }
bool        NewOrderRequest::isPostOnly()     const { return postOnly; }

RejectionReason NewOrderRequest::validate() const {
    if (orderID == 0 || qty <= 0) {
        return type == OrderType::Market ? RejectionReason::InvalidMarketOrder : RejectionReason::InvalidLimitOrder;
    }
    if (type == OrderType::Market) {
        if (priceTicks != 0 || postOnly) {
            return RejectionReason::InvalidMarketOrder;
        }
        return RejectionReason::None;
    }
    if (priceTicks <= 0) {
        return RejectionReason::InvalidLimitOrder;
    }
    if (postOnly && timeInForce != TimeInForce::GTC) {
        return RejectionReason::InvalidPostOnlyOrder;
    }
    return RejectionReason::None;
}
