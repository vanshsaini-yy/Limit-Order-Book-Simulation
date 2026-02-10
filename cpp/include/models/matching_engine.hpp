#pragma once
#include "models/order_book.hpp"
#include "models/execution_engine.hpp"
#include "policy/order_lifecycle.hpp"
#include "policy/self_trade_prevention.hpp"
#include "utils/order_utils.hpp"

class MatchingEngine {
    private:
        LimitOrderBook* orderBook;
        STPPolicy* stpPolicy;

    public:
        explicit MatchingEngine(STPPolicy* policy, LimitOrderBook* book)
            : stpPolicy(policy), orderBook(book) {}

        void applySTPPolicy(const OrderPtr &restingOrder, const OrderPtr &incomingOrder, const uint32_t incomingInitialQty) {
            STPDecision decision = stpPolicy->getDecision();
            if (decision.cancelIncoming) {
                incomingOrder->setStatus(
                    OrderLifecycle::afterCancelIncoming(incomingInitialQty, incomingOrder->getQty())
                );
            }
            if (decision.cancelResting) {
                restingOrder->setStatus(
                    OrderLifecycle::afterCancelResting(restingOrder->getStatus())
                );
                orderBook->popFront(incomingOrder->getSide());
            }
        }

        void matchOrder(const OrderPtr &incomingOrder) {
            uint32_t incomingInitialQty = incomingOrder->getQty();
            Side incomingSide = incomingOrder->getSide();
            while (orderBook->isOrderMarketable(incomingOrder)) {
                auto restingOrder = orderBook->getBestMatchedOrder(incomingSide);
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
                ExecutionEngine::executeTrade(incomingOrder, restingOrder);
                restingOrder->setStatus(
                    OrderLifecycle::afterMatching(restingInitialQty, restingOrder->getQty(), OrderType::Limit)
                );
                if (restingOrder->getQty() == 0) {
                    orderBook->popFront(incomingSide);
                }
            }
            OrderStatus finalStatus = OrderLifecycle::afterMatching(incomingInitialQty, incomingOrder->getQty(), incomingOrder->getType());
            incomingOrder->setStatus(finalStatus);
            if (finalStatus == OrderStatus::Pending || finalStatus == OrderStatus::PartiallyExecuted) {
                orderBook->addOrder(incomingOrder);
            }
        }
};