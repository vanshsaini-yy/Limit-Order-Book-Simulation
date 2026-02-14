#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include "models/order.hpp"

struct LevelInfo {
    PriceTicks price;
    Quantity totalQuantity;
    uint32_t orderCount;
};

struct SideSummaries {
    Quantity totalQuantity;
    uint32_t orderCount;
    uint64_t totalNotionalValue;
};

struct TempoMetrics {
    uint32_t executionCount;
    uint32_t cancelCount;
    uint64_t totalVolumeExecuted;
};

struct MarketStructureSnapshot {
    Timestamp timestamp;
    std::optional<PriceTicks> bestBid;
    std::optional<PriceTicks> bestAsk;
    std::optional<PriceTicks> spread;
    std::optional<PriceTicks> mid;
    SideSummaries bidSummary;
    SideSummaries askSummary;
    std::vector<LevelInfo> bidDepths;
    std::vector<LevelInfo> askDepths;
    TempoMetrics tempo;
};
