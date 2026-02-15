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
        )
            : stpPolicy(policy),
              orderBook(book),
              tradeLogger(logger),
              tradeIdGenerator(idGenerator) {}

        void applySTPPolicy(const OrderPtr &restingOrder, const OrderPtr &incomingOrder, const Quantity incomingInitialQty) {
            STPDecision decision = stpPolicy->getDecision();
            if (decision.cancelIncoming) {
                incomingOrder->setStatus(
                    OrderLifecycle::afterCancelIncoming(incomingInitialQty, incomingOrder->getQty())
                );
                orderBook->recordCancellation();
            }
            if (decision.cancelResting) {
                restingOrder->setStatus(
                    OrderLifecycle::afterCancelResting(restingOrder->getStatus())
                );
                orderBook->popFront(incomingOrder->getSide());
                orderBook->recordCancellation();
            }
        }

        void matchOrder(const OrderPtr &incomingOrder) {
            Quantity incomingInitialQty = incomingOrder->getQty();
            Side incomingSide = incomingOrder->getSide();
            while (orderBook->isOrderMarketable(incomingOrder)) {
                auto restingOrder = orderBook->getMatchedOrder(incomingSide);
                auto restingInitialQty = restingOrder->getQty();
                if (isSelfTrade(restingOrder, incomingOrder)) {
                    applySTPPolicy(restingOrder, incomingOrder, incomingInitialQty);
                    if (incomingOrder->isCancelled()) {
                        return;
                    }
                    if (restingOrder->isCancelled()) {
                        continue;
                    }
                }
                Quantity tradedQty = ExecutionEngine::executeTrade(incomingOrder, restingOrder, tradeLogger, tradeIdGenerator);
                orderBook->recordExecution(tradedQty);
                restingOrder->setStatus(
                    OrderLifecycle::afterMatching(restingInitialQty, restingOrder->getQty(), OrderType::Limit)
                );
                if (restingOrder->getQty() == 0) {
                    orderBook->popFront(incomingSide);
                }
            }
            if (incomingOrder->getType() == OrderType::Cancel) {
                orderBook->cancelOrder(incomingOrder->getOrderID());
                orderBook->recordCancellation();
            }
            OrderStatus finalStatus = OrderLifecycle::afterMatching(incomingInitialQty, incomingOrder->getQty(), incomingOrder->getType());
            incomingOrder->setStatus(finalStatus);
            if (finalStatus == OrderStatus::Pending || finalStatus == OrderStatus::PartiallyExecuted) {
                orderBook->addOrder(incomingOrder);
            }
        }
};