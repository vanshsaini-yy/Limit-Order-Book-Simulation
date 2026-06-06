#include "infra/monotonic_trade_id_generator.hpp"

MonotonicTradeIdGenerator::MonotonicTradeIdGenerator(TradeID startId)
    : currentId(startId) {}

TradeID MonotonicTradeIdGenerator::nextId() {
    return currentId.fetch_add(1, std::memory_order_relaxed);
}
