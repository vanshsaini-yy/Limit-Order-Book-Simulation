#pragma once
#include <list>
#include <memory>
#include <cstdint>
#include "models/order.hpp"

class OrderIndex{
    private:
        Side side;
        uint64_t priceTicks;
        std::list<OrderPtr>::iterator orderItr;
    public:
        OrderIndex(Side side_, uint64_t priceTicks_, std::list<OrderPtr>::iterator orderIter_)
            : side(side_), priceTicks(priceTicks_), orderItr(orderIter_) {}

        Side getSide() const { return side; }
        uint64_t getPriceTicks() const { return priceTicks; }
        std::list<OrderPtr>::iterator getOrderIter() const { return orderItr; }
};