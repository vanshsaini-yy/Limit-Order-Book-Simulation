#pragma once
#include <optional>
#include "engine/limit_order_book.hpp"
#include "engine/execution_engine.hpp"
#include "policy/stp_policy.hpp"
#include "utils/order_utils.hpp"

class TradeLogger;
class TradeIdGenerator;

class MatchingEngine {
private:
    LimitOrderBook* orderBook;
    STPPolicy* stpPolicy;
    TradeLogger* tradeLogger;
    TradeIdGenerator* tradeIdGenerator;
    std::optional<PriceTicks> maxDeviationTicks;
    std::optional<PriceTicks> lastTradedPrice;

    bool violatesPriceCollar(const OrderPtr &order) const;

public:
    MatchingEngine(
        LimitOrderBook* book,
        STPPolicy* policy,
        TradeLogger* logger = nullptr,
        TradeIdGenerator* idGenerator = nullptr,
        std::optional<PriceTicks> maxDeviationTicks = std::nullopt
    );

    void applySTPPolicy(const OrderPtr &restingOrder, const OrderPtr &incomingOrder, const Quantity incomingInitialQty);
    RejectionReason matchOrder(const OrderPtr &incomingOrder);

    std::optional<PriceTicks> getLastTradedPrice()   const;
    std::optional<PriceTicks> getMaxDeviationTicks() const;
};
