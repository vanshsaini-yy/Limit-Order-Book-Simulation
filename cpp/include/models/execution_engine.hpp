#pragma once
#include <algorithm>
#include "models/order.hpp"

class ExecutionEngine {
public:
    static Quantity executeTrade(const OrderPtr& taker, const OrderPtr& maker) {
        Quantity tradedQty = std::min(taker->getQty(), maker->getQty());
        taker->reduceQty(tradedQty);
        maker->reduceQty(tradedQty);
        return tradedQty;
    }
};
