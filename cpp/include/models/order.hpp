#pragma once
#include <cstdint>
#include <memory>

using PriceTicks = int32_t;
using Timestamp = uint64_t;
using OrderID = uint32_t;
using OwnerID = uint32_t;
using Quantity = int32_t;

enum class Side : uint8_t { Buy = 0, Sell = 1, None = 2 };
enum class OrderType : uint8_t { Limit = 0, Market = 1, Cancel = 2 };
enum class OrderStatus : uint16_t { Pending = 0, PartiallyExecuted = 1, Executed = 2, Cancelled = 3, CancelledAfterPartialExecution = 4 };

class Order {
    private:
        PriceTicks priceTicks;
        Timestamp timestamp;
        OrderID orderID;
        OwnerID ownerID;
        Quantity qty;
        Side side;
        OrderType type;
        OrderStatus status;
        OrderID linkedOrderID;

    public:
        Order(
            OrderID orderID_,
            OwnerID ownerID_,
            PriceTicks priceTicks_,
            Quantity qty_,
            Side side_,
            OrderType type_,
            Timestamp timestamp_,
            OrderID linkedOrderID_ = 0
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

        void reduceQty(Quantity qtyFilled);
        void setStatus(OrderStatus newStatus);
        bool isCancelled() const;
        bool isExecuted()  const;
};

using OrderPtr = std::shared_ptr<Order>;