#pragma once
#include <cstdint>

enum class Side : uint8_t { Buy = 0, Sell = 1 };
enum class OrderType : uint8_t { Limit = 0, Market = 1 };

class Order {
    private:
        uint64_t   id;
        uint64_t   price; 
        uint32_t   qty;
        Side       side;
        OrderType  type;
        uint64_t   timestamp;

    public:
    constexpr Order(uint64_t id_, uint64_t price_, uint32_t qty_, Side side_, OrderType type_, uint64_t timestamp_)
        : id(id_), price(price_), qty(qty_), side(side_), type(type_), timestamp(timestamp_) {}

    inline uint64_t getId() const { return id; }
    inline uint64_t getPrice() const { return price; }
    inline uint32_t getQty() const { return qty; }
    inline Side getSide() const { return side; }
    inline OrderType getType() const { return type; }
    inline uint64_t getTimestamp() const { return timestamp; }
};