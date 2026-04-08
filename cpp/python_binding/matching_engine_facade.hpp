#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "infra/binary_trade_logger.hpp"
#include "infra/monotonic_trade_id_generator.hpp"
#include "infra/trade_id_generator.hpp"
#include "infra/trade_logger.hpp"
#include "models/matching_engine.hpp"
#include "models/market_structure_snapshot.hpp"
#include "models/order_book.hpp"
#include "policy/self_trade_prevention.hpp"

class MatchingEngineFacade {
private:
    std::unique_ptr<LimitOrderBook> orderBook;
    std::unique_ptr<STPPolicy> stpPolicy;
    std::unique_ptr<TradeLogger> tradeLogger;
    std::unique_ptr<TradeIdGenerator> tradeIdGenerator;
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
        const std::string& tradeLoggerName = "none",
        const std::string& tradeLogFilePath = "trades.bin",
        TradeID tradeIdStart = 1
    );

    MatchingEngineFacade(const MatchingEngineFacade&) = delete;
    MatchingEngineFacade& operator=(const MatchingEngineFacade&) = delete;
    MatchingEngineFacade(MatchingEngineFacade&&) noexcept = default;
    MatchingEngineFacade& operator=(MatchingEngineFacade&&) noexcept = default;
    
    RejectionReason matchOrder(const OrderPtr& incomingOrder);
    MarketStructureSnapshot snapshot(Timestamp now, std::size_t depthLimit = 5) const;
};
