#pragma once
#include <cstdint>
#include <memory>

using PriceTicks = int32_t;
using Timestamp = uint64_t;
using OrderID = uint32_t;
using OwnerID = uint32_t;
using Quantity = int32_t;

enum class Side : uint8_t { 
    Buy = 0, 
    Sell = 1, 
    None = 2 
};

enum class OrderType : uint8_t {
    Limit = 0,
    Market = 1,
    Cancel = 2
};

enum class TimeInForce : uint8_t {
    GTC = 0,
    IOC = 1,
    FOK = 2
};

enum class OrderStatus : uint16_t { 
    Pending = 0, 
    PartiallyExecuted = 1, 
    Executed = 2, 
    Cancelled = 3, 
    CancelledAfterPartialExecution = 4 
};

// TODO 3: make post only something else instead of bool
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
        TimeInForce tif;
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
            TimeInForce tif_ = TimeInForce::GTC,
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
