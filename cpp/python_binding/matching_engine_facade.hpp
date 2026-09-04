#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "infra/binary_trade_logger.hpp"
#include "infra/monotonic_trade_id_generator.hpp"
#include "infra/trade_id_generator.hpp"
#include "infra/trade_logger.hpp"
#include "engine/matching_engine.hpp"
#include "models/market_structure_snapshot.hpp"
#include "engine/limit_order_book.hpp"
#include "policy/stp_policy.hpp"
#include "utils/constants.hpp"

class MatchingEngineFacade {
private:
    double tickSize;
    double lotSize;
    double timeInterval;
    std::unique_ptr<LimitOrderBook> orderBook;
    std::unique_ptr<STPPolicy> stpPolicy;
    std::unique_ptr<TradeIdGenerator> tradeIdGenerator;
    std::unique_ptr<TradeLogger> tradeLogger;
    std::unique_ptr<MatchingEngine> matchingEngine;

    static std::unique_ptr<STPPolicy> makeSTPPolicy(
        const std::string& stpPolicyName
    );

    static std::unique_ptr<TradeIdGenerator> makeTradeIdGenerator(
        const std::string& tradeIdGeneratorName,
        TradeID tradeIdStart
    );

    static std::unique_ptr<TradeLogger> makeTradeLogger(
        const std::string& tradeLoggerName,
        const std::string& tradeLogFilePath
    );

public:
    MatchingEngineFacade(
        const std::string& stpPolicyName,
        const std::string& tradeIdGeneratorName = "none",
        TradeID tradeIdStart = 1,
        const std::string& tradeLoggerName = "none",
        const std::string& tradeLogFilePath = "trades.bin",
        double tickSize_ = DEFAULT_TICK_SIZE,
        double lotSize_ = DEFAULT_LOT_SIZE,
        double timeInterval_ = DEFAULT_TIME_INTERVAL,
        std::optional<PriceTicks> deviationTicks = std::nullopt
    );

    double getTickSize()     const { return tickSize; }
    double getLotSize()      const { return lotSize; }
    double getTimeInterval() const { return timeInterval; }

    MatchingEngineFacade(const MatchingEngineFacade&) = delete;
    MatchingEngineFacade& operator=(const MatchingEngineFacade&) = delete;
    MatchingEngineFacade(MatchingEngineFacade&&) noexcept = default;
    MatchingEngineFacade& operator=(MatchingEngineFacade&&) noexcept = default;
    
    RejectionReason matchOrder(const OrderPtr& incomingOrder);
    MarketStructureSnapshot snapshot(Timestamp now, std::size_t depthLimit = 5) const;
};
