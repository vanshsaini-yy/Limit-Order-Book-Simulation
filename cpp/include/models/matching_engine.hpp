#pragma once
#include "models/order_book.hpp"
#include "models/execution_engine.hpp"
#include "policy/self_trade_prevention.hpp"
#include "utils/order_utils.hpp"

class TradeLogger;
class TradeIdGenerator;

class MatchingEngine {
    private:
        LimitOrderBook* orderBook;
        STPPolicy* stpPolicy;
        TradeLogger* tradeLogger;
        TradeIdGenerator* tradeIdGenerator;

    public:
        explicit MatchingEngine(
            STPPolicy* policy,
            LimitOrderBook* book,
            TradeLogger* logger = nullptr,
            TradeIdGenerator* idGenerator = nullptr
        );

        void applySTPPolicy(const OrderPtr &restingOrder, const OrderPtr &incomingOrder, const Quantity incomingInitialQty);
        RejectionReason matchOrder(const OrderPtr &incomingOrder);
};
