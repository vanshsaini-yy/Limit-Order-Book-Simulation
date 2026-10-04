#include "engine/matching_engine.hpp"

namespace {
ModifyPolicy* fallbackModifyPolicy() {
    static DefaultModifyPolicy policy;
    return &policy;
}
}

MatchingEngine::MatchingEngine(
    LimitOrderBook* book,
    STPPolicy* policy,
    TradeLogger* logger,
    TradeIdGenerator* idGenerator,
    std::optional<PriceTicks> deviationTicks,
    ModifyPolicy* modifyPolicy_
)
    : orderBook(book),
      stpPolicy(policy),
      tradeLogger(logger),
      tradeIdGenerator(idGenerator),
      maxDeviationTicks(deviationTicks),
      modifyPolicy(modifyPolicy_ != nullptr ? modifyPolicy_ : fallbackModifyPolicy()) {}

std::optional<PriceTicks> MatchingEngine::getLastTradedPrice()   const { return lastTradedPrice; }
std::optional<PriceTicks> MatchingEngine::getMaxDeviationTicks() const { return maxDeviationTicks; }

bool MatchingEngine::violatesPriceCollar(const Order& order) const {
    if (!maxDeviationTicks.has_value() || order.getType() != OrderType::Limit) {
        return false;
    }
    std::optional<PriceTicks> reference = lastTradedPrice.has_value() ? lastTradedPrice : orderBook->getMidPrice();
    if (!reference.has_value()) {
        return false;
    }
    PriceTicks lowerBound = *reference - *maxDeviationTicks;
    PriceTicks upperBound = *reference + *maxDeviationTicks;
    return order.getPriceTicks() < lowerBound || order.getPriceTicks() > upperBound;
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

RejectionReason MatchingEngine::checkBeforeMatching(const OrderPtr &incomingOrder) const {
    if (incomingOrder->isPostOnly() && orderBook->isOrderMarketable(incomingOrder)) {
        incomingOrder->setStatus(OrderStatus::Cancelled);
        return RejectionReason::PostOnlyWouldCross;
    }

    if (incomingOrder->getTimeInForce() == TimeInForce::FOK && !orderBook->isFOKFillable(incomingOrder, stpPolicy->getDecision())) {
        incomingOrder->setStatus(OrderStatus::Cancelled);
        return RejectionReason::FOKInsufficientLiquidity;
    }

    if (violatesPriceCollar(*incomingOrder)) {
        incomingOrder->setStatus(OrderStatus::Cancelled);
        return RejectionReason::PriceCollarViolation;
    }

    return RejectionReason::None;
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

    RejectionReason checkResult = checkBeforeMatching(incomingOrder);
    if (checkResult != RejectionReason::None) {
        return checkResult;
    }

    return executeMatching(incomingOrder);
}

RejectionReason MatchingEngine::executeMatching(const OrderPtr &incomingOrder) {
    Quantity incomingInitialQty = incomingOrder->getOriginalQty();
    Side incomingSide = incomingOrder->getSide();

    while (orderBook->isOrderMarketable(incomingOrder)) {
        OrderPtr restingOrder = orderBook->getMatchedOrder(incomingSide);
        if (!restingOrder) {
            incomingOrder->setStatus(OrderStatus::Cancelled);
            // TODO: why do we return invariant violation here
            return RejectionReason::OrderBookInvariantViolation;
        }
        Quantity restingInitialQty = restingOrder->getQty();

        if (isSelfTrade(*restingOrder, *incomingOrder)) {
            applySTPPolicy(restingOrder, incomingOrder, incomingInitialQty);
            if (incomingOrder->isCancelled()) {
                return RejectionReason::SelfTradePrevention;
            }
            if (restingOrder->isCancelled()) {
                continue;
            }
        }

        Quantity tradedQty = ExecutionEngine::executeTrade(*incomingOrder, *restingOrder, tradeLogger, tradeIdGenerator);
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

RejectionReason MatchingEngine::submit(const CancelRequest &request) {
    RejectionReason validationResult = request.validate();
    if (validationResult != RejectionReason::None) {
        return validationResult;
    }
    RejectionReason cancelResult = orderBook->cancelOrder(request.getTargetOrderID(), request.getOwnerID());
    if (cancelResult != RejectionReason::None) {
        return cancelResult;
    }
    orderBook->recordCancellation();
    return RejectionReason::None;
}

RejectionReason MatchingEngine::submit(const ModifyRequest &request) {
    RejectionReason validationResult = request.validate();
    if (validationResult != RejectionReason::None) {
        return validationResult;
    }

    OrderPtr resting = orderBook->getOrder(request.getTargetOrderID());
    if (!resting || resting->getOwnerID() != request.getOwnerID()) {
        return RejectionReason::OrderToBeModifiedDoesNotExist;
    }

    std::optional<PriceTicks> newPriceTicks = request.getNewPriceTicks();
    std::optional<Quantity> newOriginalQty = request.getNewOriginalQty();
    bool priceUnchanged = !newPriceTicks.has_value() || *newPriceTicks == resting->getPriceTicks();
    bool quantityUnchanged = !newOriginalQty.has_value() || *newOriginalQty == resting->getOriginalQty();
    if (priceUnchanged && quantityUnchanged) {
        return RejectionReason::NoOpModify;
    }

    if (newOriginalQty.has_value()) {
        if (*newOriginalQty < resting->getFilledQty()) {
            return RejectionReason::ModifyQuantityBelowFilled;
        }
        if (*newOriginalQty == resting->getFilledQty()) {
            RejectionReason cancelResult = orderBook->cancelOrder(request.getTargetOrderID(), request.getOwnerID());
            if (cancelResult != RejectionReason::None) {
                return cancelResult;
            }
            orderBook->recordCancellation();
            return RejectionReason::None;
        }
    }

    ModifyDecision decision = modifyPolicy->getDecision(*resting, request);

    if (!decision.losesPriority) {
        if (newOriginalQty.has_value()) {
            resting->modifyOriginalQty(*newOriginalQty);
        }
        return RejectionReason::None;
    }

    PriceTicks replacementPriceTicks = newPriceTicks.value_or(resting->getPriceTicks());

    OrderPtr replacement = std::make_shared<Order>(
        resting->getOrderID(),
        resting->getOwnerID(),
        replacementPriceTicks,
        resting->getOriginalQty(),
        resting->getSide(),
        resting->getType(),
        request.getTimestamp(),
        resting->getTimeInForce(),
        resting->isPostOnly()
    );
    replacement->reduceQty(resting->getFilledQty());
    if (newOriginalQty.has_value()) {
        replacement->modifyOriginalQty(*newOriginalQty);
    }

    RejectionReason checkResult = checkBeforeMatching(replacement);
    if (checkResult != RejectionReason::None) {
        return checkResult;
    }

    RejectionReason cancelResult = orderBook->cancelOrder(request.getTargetOrderID(), request.getOwnerID());
    if (cancelResult != RejectionReason::None) {
        return cancelResult;
    }

    return executeMatching(replacement);
}
