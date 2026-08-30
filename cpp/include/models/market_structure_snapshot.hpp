#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include "models/order.hpp"

using Count = uint32_t;

// TODO: use better datatypes for aggregates
struct LevelInfo {
    PriceTicks price;
    Quantity totalQuantity;
    Count orderCount;
};

struct SideSummary {
    Quantity totalQuantity;
    Count totalOrderCount;
    uint64_t totalNotionalValue;
};

struct TempoMetrics {
    Count tradeExecutionCount;
    Count orderCancellationCount;
    uint64_t totalVolumeTraded;
};

struct MarketStructureSnapshot {
    Timestamp timestamp;
    std::optional<PriceTicks> bestBid;
    std::optional<PriceTicks> bestAsk;
    std::optional<PriceTicks> spread;
    std::optional<PriceTicks> mid;
    SideSummary bidSummary;
    SideSummary askSummary;
    std::vector<LevelInfo> bidDepths;
    std::vector<LevelInfo> askDepths;
    TempoMetrics tempo;
};
