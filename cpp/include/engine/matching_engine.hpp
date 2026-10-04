#pragma once
#include <optional>
#include "engine/limit_order_book.hpp"
#include "engine/execution_engine.hpp"
#include "policy/stp_policy.hpp"
#include "policy/modify_policy.hpp"
#include "utils/order_utils.hpp"
#include "models/cancel_request.hpp"
#include "models/modify_request.hpp"

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
    ModifyPolicy* modifyPolicy;

    bool violatesPriceCollar(const Order& order) const;
    void applySTPPolicy(const OrderPtr &restingOrder, const OrderPtr &incomingOrder, const Quantity incomingInitialQty);
    RejectionReason checkBeforeMatching(const OrderPtr &incomingOrder) const;
    RejectionReason executeMatching(const OrderPtr &incomingOrder);

public:
    MatchingEngine(
        LimitOrderBook* book,
        STPPolicy* policy,
        TradeLogger* logger = nullptr,
        TradeIdGenerator* idGenerator = nullptr,
        std::optional<PriceTicks> maxDeviationTicks = std::nullopt,
        ModifyPolicy* modifyPolicy = nullptr
    );

    RejectionReason matchOrder(const OrderPtr &incomingOrder);
    RejectionReason submit(const CancelRequest &request);
    RejectionReason submit(const ModifyRequest &request);

    std::optional<PriceTicks> getLastTradedPrice()   const;
    std::optional<PriceTicks> getMaxDeviationTicks() const;
};
