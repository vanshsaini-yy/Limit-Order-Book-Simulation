#pragma once
#include <memory>
#include "models/types.hpp"
#include "models/side.hpp"
#include "models/order_type.hpp"
#include "models/time_in_force.hpp"
#include "models/order_status.hpp"

class Order {
private:
    OrderID orderID;
    OwnerID ownerID;
    PriceTicks priceTicks;
    Quantity qty;
    Side side;
    OrderType type;
    Timestamp timestamp;
    OrderStatus status;
    OrderID linkedOrderID;
    TimeInForce timeInForce;
    bool postOnly;

public:
    Order(
        OrderID orderID_,
        OwnerID ownerID_,
        PriceTicks priceTicks_,
        Quantity qty_,
        Side side_,
        OrderType type_,
        Timestamp timestamp_,
        OrderID linkedOrderID_ = 0,
        TimeInForce timeInForce_ = TimeInForce::GTC,
        bool postOnly_ = false
    );

    OrderID     getOrderID()       const;
    OwnerID     getOwnerID()       const;
    PriceTicks  getPriceTicks()    const;
    Quantity    getQty()           const;
    Side        getSide()          const;
    OrderType   getType()          const;
    Timestamp   getTimestamp()     const;
    OrderStatus getStatus()        const;
    OrderID     getLinkedOrderID() const;
    TimeInForce getTimeInForce()   const;

    void reduceQty(Quantity qtyFilled);
    void setStatus(OrderStatus newStatus);

    bool isCancelled() const;
    bool isExecuted()  const;
    bool isPostOnly()  const;
};

using OrderPtr = std::shared_ptr<Order>;
