#pragma once
#include <cstdint>
#include <memory>
#include "models/side.hpp"
#include "models/order_type.hpp"
#include "models/time_in_force.hpp"
#include "models/order_status.hpp"

// PriceTicks and Quantity are deliberately signed: unsigned would wrap a
// caller's negative value to a large positive one that passes OrderValidator.
using PriceTicks = int32_t;
using Timestamp = uint64_t;
using OrderID = uint32_t;
using OwnerID = uint32_t;
using Quantity = int32_t;

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

// TODO: think and make the right decision for this type here and across call sites
using OrderPtr = std::shared_ptr<Order>;
