#pragma once
#include <algorithm>
#include "models/order.hpp"

class ExecutionEngine {
public:
    static uint32_t executeTrade(const OrderPtr& taker, const OrderPtr& maker) {
        uint32_t tradedQty = std::min(taker->getQty(), maker->getQty());
        taker->reduceQty(tradedQty);
        maker->reduceQty(tradedQty);
        return tradedQty;
    }
};
