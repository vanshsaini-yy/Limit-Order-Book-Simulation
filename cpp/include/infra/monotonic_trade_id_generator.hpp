#pragma once
#include <atomic>
#include "infra/trade_id_generator.hpp"

class MonotonicTradeIdGenerator : public TradeIdGenerator {
    private:
        std::atomic<TradeID> currentId;

    public:
        explicit MonotonicTradeIdGenerator(TradeID startId = 1)
            : currentId(startId) {}

        TradeID nextId() override {
            return currentId.fetch_add(1, std::memory_order_relaxed);
        }
};
