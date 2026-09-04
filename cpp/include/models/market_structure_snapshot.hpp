#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include "models/order.hpp"

using Count = uint32_t;
using AggregateQuantity = uint64_t;
using AggregateValue = uint64_t;

struct LevelInfo {
    PriceTicks price;
    AggregateQuantity totalQuantity;
    Count orderCount;
};

struct SideSummary {
    AggregateQuantity totalQuantity;
    Count totalOrderCount;
    AggregateValue totalNotionalValue;
};

struct TempoMetrics {
    Count tradeExecutionCount;
    Count orderCancellationCount;
    AggregateQuantity totalVolumeTraded;
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
