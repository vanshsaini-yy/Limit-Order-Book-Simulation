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
            }

            if (decision.cancelResting) {
                restingOrder->setStatus(
                    OrderLifecycle::afterCancelResting(restingOrder->getStatus())
                );
                orderBook->popFront(incomingOrder->getSide());
            }
        }

        RejectionReason matchOrder(const OrderPtr &incomingOrder) {
            RejectionReason validationResult = OrderValidator::validateBeforeMatching(incomingOrder);
            if (validationResult != RejectionReason::None) {
                if (incomingOrder) {
                    incomingOrder->setStatus(OrderStatus::Cancelled);
                }
                return validationResult;
            }

            if (orderBook->doesOrderExist(incomingOrder->getOrderID())) {
                return RejectionReason::OrderToBeAddedAlreadyExists;
            }

            Quantity incomingInitialQty = incomingOrder->getQty();
            Side incomingSide = incomingOrder->getSide();

            while (orderBook->isOrderMarketable(incomingOrder)) {
                OrderPtr restingOrder = orderBook->getMatchedOrder(incomingSide);
                Quantity restingInitialQty = restingOrder->getQty();

                if (isSelfTrade(restingOrder, incomingOrder)) {
                    applySTPPolicy(restingOrder, incomingOrder, incomingInitialQty);
                    if (incomingOrder->isCancelled()) {
                        return RejectionReason::None;
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
                RejectionReason cancelResult = orderBook->cancelOrder(incomingOrder->getLinkedOrderID());
                if (cancelResult != RejectionReason::None) {
                    incomingOrder->setStatus(OrderStatus::Cancelled);
                    return cancelResult;
                }
                orderBook->recordCancellation();
            }

            OrderStatus finalStatus = OrderLifecycle::afterMatching(incomingInitialQty, incomingOrder->getQty(), incomingOrder->getType());
            incomingOrder->setStatus(finalStatus);

            if (finalStatus == OrderStatus::Pending || finalStatus == OrderStatus::PartiallyExecuted) {
                RejectionReason addResult = orderBook->addOrder(incomingOrder);
                if (addResult != RejectionReason::None) {
                    incomingOrder->setStatus(
                        OrderLifecycle::afterCancelResting(incomingOrder->getStatus())
                    );
                    return addResult;
                }
            }
            return RejectionReason::None;
        }
};