#pragma once
#include <cstdint>

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
        Side  side;
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
        )
        :   orderID(orderID_), 
            ownerID(ownerID_), 
            priceTicks(priceTicks_),
            qty(qty_), 
            side(side_),
            type(type_), 
            timestamp(timestamp_), 
            status(OrderStatus::Pending),
            linkedOrderID(linkedOrderID_) {}

        inline OrderID getOrderID() const { return orderID; }
        inline OwnerID getOwnerID() const { return ownerID; }
        inline PriceTicks getPriceTicks() const { return priceTicks; }
        inline Quantity getQty() const { return qty; }
        inline Side getSide() const { return side; }
        inline OrderType getType() const { return type; }
        inline Timestamp getTimestamp() const { return timestamp; }
        inline OrderStatus getStatus() const { return status; }
        inline OrderID getLinkedOrderID() const { return linkedOrderID; }

        inline void reduceQty(Quantity qtyFilled) { qty -= qtyFilled; }
        inline void setStatus(OrderStatus newStatus) { status = newStatus; }
        inline bool isCancelled() const { return status == OrderStatus::Cancelled || status == OrderStatus::CancelledAfterPartialExecution; }
        inline bool isExecuted() const { return status == OrderStatus::Executed; }
};

using OrderPtr = Order*;