#include "matching_engine_facade.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {
std::string normalize(const std::string& value) {
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return normalized;
}
}

std::unique_ptr<STPPolicy> MatchingEngineFacade::makeSTPPolicy(const std::string& stpPolicyName) {
    const std::string key = normalize(stpPolicyName);
    if (key == "cancel_both") {
        return std::make_unique<CancelBothSTP>();
    }
    if (key == "cancel_incoming") {
        return std::make_unique<CancelIncomingSTP>();
    }
    if (key == "cancel_resting") {
        return std::make_unique<CancelRestingSTP>();
    }
    throw std::invalid_argument("Unknown STP policy: " + stpPolicyName);
}

std::unique_ptr<ModifyPolicy> MatchingEngineFacade::makeModifyPolicy(const std::string& modifyPolicyName) {
    const std::string key = normalize(modifyPolicyName);
    if (key == "mp1") {
        return std::make_unique<MP1>();
    }
    if (key == "mp2") {
        return std::make_unique<MP2>();
    }
    throw std::invalid_argument("Unknown modify policy: " + modifyPolicyName);
}

std::unique_ptr<TradeIdGenerator> MatchingEngineFacade::makeTradeIdGenerator(
    const std::string& tradeIdGeneratorName,
    TradeID tradeIdStart
) {
    const std::string key = normalize(tradeIdGeneratorName);
    if (key == "none") {
        return nullptr;
    }
    if (key == "monotonic") {
        return std::make_unique<MonotonicTradeIdGenerator>(tradeIdStart);
    }
    throw std::invalid_argument("Unknown trade ID generator: " + tradeIdGeneratorName);
}

std::unique_ptr<TradeLogger> MatchingEngineFacade::makeTradeLogger(
    const std::string& tradeLoggerName,
    const std::string& tradeLogFilePath
) {
    const std::string key = normalize(tradeLoggerName);
    if (key == "none") {
        return nullptr;
    }
    if (key == "binary") {
        return std::make_unique<BinaryTradeLogger>(tradeLogFilePath);
    }
    throw std::invalid_argument("Unknown trade logger: " + tradeLoggerName);
}

MatchingEngineFacade::MatchingEngineFacade(
    const std::string& stpPolicyName,
    const std::string& tradeIdGeneratorName,
    TradeID tradeIdStart,
    const std::string& tradeLoggerName,
    const std::string& tradeLogFilePath,
    double tickSize_,
    double lotSize_,
    double timeInterval_,
    std::optional<PriceTicks> deviationTicks,
    const std::string& modifyPolicyName
)
    : tickSize(tickSize_),
      lotSize(lotSize_),
      timeInterval(timeInterval_),
      orderBook(std::make_unique<LimitOrderBook>()),
      stpPolicy(makeSTPPolicy(stpPolicyName)),
      modifyPolicy(makeModifyPolicy(modifyPolicyName)),
      tradeIdGenerator(makeTradeIdGenerator(tradeIdGeneratorName, tradeIdStart)),
      tradeLogger(makeTradeLogger(tradeLoggerName, tradeLogFilePath)),
      matchingEngine(std::make_unique<MatchingEngine>(
          orderBook.get(),
          stpPolicy.get(),
          tradeLogger.get(),
          tradeIdGenerator.get(),
          deviationTicks,
          modifyPolicy.get())) {
    if (tradeLogger != nullptr && tradeIdGenerator == nullptr) {
        throw std::invalid_argument("tradeLogger requires a tradeIdGenerator");
    }
}

SubmitResult MatchingEngineFacade::submit(const NewOrderRequest& request) {
    return matchingEngine->submit(request);
}

SubmitResult MatchingEngineFacade::cancel(const CancelRequest& request) {
    return matchingEngine->submit(request);
}

SubmitResult MatchingEngineFacade::modify(const ModifyRequest& request) {
    return matchingEngine->submit(request);
}

MarketStructureSnapshot MatchingEngineFacade::snapshot(Timestamp now, std::size_t depthLimit) const {
    return orderBook->snapshot(now, depthLimit);
}
