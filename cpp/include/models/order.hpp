#pragma once
#include <cstdint>

enum class Side : uint8_t { Buy = 0, Sell = 1 };
enum class OrderType : uint8_t { Limit = 0, Market = 1 };
enum class OrderStatus : uint16_t { Pending = 0, PartiallyExecuted = 1, Executed = 2, Cancelled = 3, CancelledAfterPartialExecution = 4 };

class Order {
    private:
        uint64_t priceTicks;
        uint64_t timestamp;
        uint32_t orderID;
        uint32_t ownerID;
        uint32_t qty;
        Side  side;
        OrderType type;
        OrderStatus status;

    public:
        Order(
            uint32_t orderID_, 
            uint32_t ownerID_, 
            uint64_t priceTicks_, 
            uint32_t qty_, 
            Side side_, 
            OrderType type_, 
            uint64_t timestamp_
        )
        :   orderID(orderID_), 
            ownerID(ownerID_), 
            priceTicks(priceTicks_),
            qty(qty_), 
            side(side_),
            type(type_), 
            timestamp(timestamp_), 
            status(OrderStatus::Pending) {}

        inline uint32_t getOrderID() const { return orderID; }
        inline uint32_t getOwnerID() const { return ownerID; }
        inline uint64_t getPriceTicks() const { return priceTicks; }
        inline uint32_t getQty() const { return qty; }
        inline Side getSide() const { return side; }
        inline OrderType getType() const { return type; }
        inline uint64_t getTimestamp() const { return timestamp; }
        inline OrderStatus getStatus() const { return status; }

        inline void reduceQty(uint32_t qtyFilled) { qty -= qtyFilled; }
        inline void setStatus(OrderStatus newStatus) { status = newStatus; }
        inline bool isCancelled() const { return status == OrderStatus::Cancelled || status == OrderStatus::CancelledAfterPartialExecution; }
        inline bool isExecuted() const { return status == OrderStatus::Executed; }
};

using OrderPtr = Order*;