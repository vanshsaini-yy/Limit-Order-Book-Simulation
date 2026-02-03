#pragma once
#include <list>
#include <memory>
#include <cstdint>
#include "models/order.hpp"

using OrderPtr = Order*;

class OrderIndex{
    private:
        uint8_t isBuy;
        uint64_t priceTicks;
        std::list<OrderPtr>::iterator orderItr;
    public:
        OrderIndex(uint8_t isBuy_, uint64_t priceTicks_, std::list<OrderPtr>::iterator orderIter_)
            : isBuy(isBuy_), priceTicks(priceTicks_), orderItr(orderIter_) {}

        uint8_t getIsBuy() const { return isBuy; }
        uint64_t getPriceTicks() const { return priceTicks; }
        std::list<OrderPtr>::iterator getOrderIter() const { return orderItr; }
};