#include "models/matching_engine.hpp"

MatchingEngine::MatchingEngine(
    LimitOrderBook* book,
    STPPolicy* policy,
    TradeLogger* logger,
    TradeIdGenerator* idGenerator,
    std::optional<PriceTicks> deviationTicks
)
    : orderBook(book),
      stpPolicy(policy),
      tradeLogger(logger),
      tradeIdGenerator(idGenerator),
      maxDeviationTicks(deviationTicks) {}

std::optional<PriceTicks> MatchingEngine::getLastTradedPrice()   const { return lastTradedPrice; }
std::optional<PriceTicks> MatchingEngine::getMaxDeviationTicks() const { return maxDeviationTicks; }

bool MatchingEngine::violatesPriceCollar(const OrderPtr &order) const {
    if (!maxDeviationTicks.has_value() || order->getType() != OrderType::Limit) {
        return false;
    }
    std::optional<PriceTicks> reference = lastTradedPrice.has_value() ? lastTradedPrice : orderBook->getMidPrice();
    if (!reference.has_value()) {
        return false;
    }
    PriceTicks lowerBound = *reference - *maxDeviationTicks;
    PriceTicks upperBound = *reference + *maxDeviationTicks;
    return order->getPriceTicks() < lowerBound || order->getPriceTicks() > upperBound;
}

void MatchingEngine::applySTPPolicy(const OrderPtr &restingOrder, const OrderPtr &incomingOrder, const Quantity incomingInitialQty) {
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

RejectionReason MatchingEngine::matchOrder(const OrderPtr &incomingOrder) {
    RejectionReason validationResult = OrderValidator::validateBeforeMatching(incomingOrder);
    if (validationResult != RejectionReason::None) {
        if (incomingOrder) {
            incomingOrder->setStatus(OrderStatus::Cancelled);
        }
        return validationResult;
    }

    if (orderBook->doesOrderExist(incomingOrder->getOrderID())) {
        return RejectionReason::DuplicateOrderID;
    }

    if (incomingOrder->isPostOnly() && orderBook->isOrderMarketable(incomingOrder)) {
        incomingOrder->setStatus(OrderStatus::Cancelled);
        return RejectionReason::PostOnlyWouldCross;
    }

    if (incomingOrder->getTimeInForce() == TimeInForce::FOK && !orderBook->isFOKFillable(incomingOrder)) {
        incomingOrder->setStatus(OrderStatus::Cancelled);
        return RejectionReason::FOKInsufficientLiquidity;
    }

    if (violatesPriceCollar(incomingOrder)) {
        incomingOrder->setStatus(OrderStatus::Cancelled);
        return RejectionReason::PriceCollarViolation;
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
        if (tradedQty > 0) {
            lastTradedPrice = restingOrder->getPriceTicks();
        }

        restingOrder->setStatus(
            OrderLifecycle::afterMatching(restingInitialQty, restingOrder->getQty(), OrderType::Limit)
        );
        if (restingOrder->getQty() == 0) {
            orderBook->popFront(incomingSide);
        }
    }

    if (incomingOrder->getType() == OrderType::Cancel) {
        RejectionReason cancelResult = orderBook->cancelOrder(incomingOrder->getLinkedOrderID(), incomingOrder->getOwnerID());
        if (cancelResult != RejectionReason::None) {
            incomingOrder->setStatus(OrderStatus::Cancelled);
            return cancelResult;
        }
        orderBook->recordCancellation();
    }

    OrderStatus finalStatus = OrderLifecycle::afterMatching(incomingInitialQty, incomingOrder->getQty(), incomingOrder->getType());
    incomingOrder->setStatus(finalStatus);

    bool wouldRest = finalStatus == OrderStatus::Pending || finalStatus == OrderStatus::PartiallyExecuted;

    if (wouldRest && incomingOrder->getTimeInForce() != TimeInForce::GTC) {
        incomingOrder->setStatus(
            OrderLifecycle::afterCancelIncoming(incomingInitialQty, incomingOrder->getQty())
        );
        return RejectionReason::None;
    }

    if (wouldRest) {
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
