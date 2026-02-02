#pragma once
#include <cstdint>

enum class Side : uint8_t { Buy = 0, Sell = 1 };
enum class OrderType : uint8_t { Limit = 0, Market = 1 };

class Order {
    private:
        uint64_t priceTicks;
        uint64_t timestamp;
        uint32_t orderID;
        uint32_t ownerID;
        uint32_t qty;
        uint8_t  side;
        uint8_t  type;
        uint16_t flags;

    public:
        constexpr Order(uint32_t orderID_, uint32_t ownerID_, uint64_t priceTicks_, uint32_t qty_,
                        Side side_, OrderType type_, uint64_t timestamp_) noexcept
            :   orderID(orderID_), 
                ownerID(ownerID_), 
                priceTicks(priceTicks_),
                qty(qty_), 
                side(static_cast<uint8_t>(side_)),
                type(static_cast<uint8_t>(type_)), 
                timestamp(timestamp_), 
                flags(0) {}

    inline uint32_t getOrderID() const { return orderID; }
    inline uint32_t getOwnerID() const { return ownerID; }
    inline uint64_t getPriceTicks() const { return priceTicks; }
    inline uint32_t getQty() const { return qty; }
    inline Side getSide() const { return static_cast<Side>(side); }
    inline OrderType getType() const { return static_cast<OrderType>(type); }
    inline uint64_t getTimestamp() const { return timestamp; }
};