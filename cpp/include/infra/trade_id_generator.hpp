#pragma once
#include "models/trade.hpp"

class TradeIdGenerator {
    public:
        virtual ~TradeIdGenerator() = default;
        virtual TradeID nextId() = 0;
};
