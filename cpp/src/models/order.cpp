#include "models/order.hpp"

Order::Order(
    OrderID orderID_,
    OwnerID ownerID_,
    PriceTicks priceTicks_,
    Quantity qty_,
    Side side_,
    OrderType type_,
    Timestamp timestamp_,
    OrderID linkedOrderID_,
    TimeInForce timeInForce_,
    bool postOnly_
)
:   orderID(orderID_),
    ownerID(ownerID_),
    priceTicks(priceTicks_),
    qty(qty_),
    side(side_),
    type(type_),
    timestamp(timestamp_),
    status(OrderStatus::Pending),
    linkedOrderID(linkedOrderID_),
    timeInForce(timeInForce_),
    postOnly(postOnly_) {}

OrderID     Order::getOrderID()       const { return orderID; }
OwnerID     Order::getOwnerID()       const { return ownerID; }
PriceTicks  Order::getPriceTicks()    const { return priceTicks; }
Quantity    Order::getQty()           const { return qty; }
Side        Order::getSide()          const { return side; }
OrderType   Order::getType()          const { return type; }
Timestamp   Order::getTimestamp()     const { return timestamp; }
OrderStatus Order::getStatus()        const { return status; }
OrderID     Order::getLinkedOrderID() const { return linkedOrderID; }
TimeInForce Order::getTimeInForce()   const { return timeInForce; }

void Order::reduceQty(Quantity qtyFilled)    { qty -= qtyFilled; }
void Order::setStatus(OrderStatus newStatus) { status = newStatus; }

bool Order::isCancelled() const { return status == OrderStatus::Cancelled || status == OrderStatus::CancelledAfterPartialExecution; }
bool Order::isExecuted()  const { return status == OrderStatus::Executed; }
bool Order::isPostOnly()  const { return postOnly; }
