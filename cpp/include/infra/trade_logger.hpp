#pragma once
#include "models/trade.hpp"

class TradeLogger {
    public:
    virtual ~TradeLogger() = default;
        virtual void log(const Trade& trade) = 0;
        virtual void flush() = 0;
        virtual void close() = 0;
};
