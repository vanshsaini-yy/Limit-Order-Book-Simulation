#pragma once
#include "models/request.hpp"
#include "models/side.hpp"
#include "models/order_type.hpp"
#include "models/time_in_force.hpp"

class NewOrderRequest : public IRequest {
private:
    OrderID orderID;
    PriceTicks priceTicks;
    Quantity qty;
    Side side;
    OrderType type;
    TimeInForce timeInForce;
    bool postOnly;

public:
    NewOrderRequest(
        RequestID requestId_,
        OwnerID ownerID_,
        Timestamp timestamp_,
        OrderID orderID_,
        PriceTicks priceTicks_,
        Quantity qty_,
        Side side_,
        OrderType type_,
        TimeInForce timeInForce_ = TimeInForce::GTC,
        bool postOnly_ = false
    );

    OrderID     getOrderID()     const;
    PriceTicks  getPriceTicks()  const;
    Quantity    getQty()         const;
    Side        getSide()        const;
    OrderType   getType()        const;
    TimeInForce getTimeInForce() const;
    bool        isPostOnly()     const;

    RejectionReason validate() const override;
};
