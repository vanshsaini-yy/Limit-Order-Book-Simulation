#pragma once
#include <list>
#include <memory>
#include <cstdint>
#include "models/order.hpp"

class OrderIndex{
    private:
        uint8_t isBuy;
        uint64_t priceTicks;
        std::list<std::shared_ptr<Order>>::iterator orderIter;
    public:
        OrderIndex(uint8_t isBuy_, uint64_t priceTicks_, std::list<std::shared_ptr<Order>>::iterator orderIter_)
            : isBuy(isBuy_), priceTicks(priceTicks_), orderIter(orderIter_) {}

        uint8_t getIsBuy() const { return isBuy; }
        uint64_t getPriceTicks() const { return priceTicks; }
        std::list<std::shared_ptr<Order>>::iterator getOrderIter() const { return orderIter; }
};