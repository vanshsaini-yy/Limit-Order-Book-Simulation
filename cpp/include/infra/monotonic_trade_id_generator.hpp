#pragma once
#include <atomic>
#include "infra/trade_id_generator.hpp"

class MonotonicTradeIdGenerator : public TradeIdGenerator {
private:
    std::atomic<TradeID> currentId;

public:
    explicit MonotonicTradeIdGenerator(TradeID startId = 1);
    TradeID nextId() override;
};
