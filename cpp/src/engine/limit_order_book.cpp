#include "engine/limit_order_book.hpp"

bool LimitOrderBook::doesOrderExist(OrderID orderId) const {
    return orderIDMap.contains(orderId);
}

std::optional<PriceTicks> LimitOrderBook::getBestBid() const {
    if (bids.empty()) return std::nullopt;
    return bids.begin()->first;
}

std::optional<PriceTicks> LimitOrderBook::getBestAsk() const {
    if (asks.empty()) return std::nullopt;
    return asks.begin()->first;
}

std::optional<PriceTicks> LimitOrderBook::getMidPrice() const {
    auto bestBid = getBestBid();
    auto bestAsk = getBestAsk();
    if (!bestBid.has_value() || !bestAsk.has_value()) return std::nullopt;
    return (*bestBid + *bestAsk) / 2;
}

std::optional<PriceTicks> LimitOrderBook::getSpread() const {
    auto bestBid = getBestBid();
    auto bestAsk = getBestAsk();
    if (!bestBid.has_value() || !bestAsk.has_value()) return std::nullopt;
    return *bestAsk - *bestBid;
}

Count LimitOrderBook::getTradeExecutionCount()           const { return tradeExecutionCount; }
Count LimitOrderBook::getOrderCancellationCount()        const { return orderCancellationCount; }
AggregateQuantity LimitOrderBook::getTotalVolumeTraded() const { return totalVolumeTraded; }

void LimitOrderBook::recordExecution(Quantity tradedQty) {
    if (tradedQty > 0) {
        ++tradeExecutionCount;
        totalVolumeTraded += static_cast<AggregateQuantity>(tradedQty);
    }
}

void LimitOrderBook::recordCancellation() {
    ++orderCancellationCount;
}

RejectionReason LimitOrderBook::addOrder(const OrderPtr &order) {
    RejectionReason validationResult = OrderValidator::validateBeforeAddingOrRemoving(order);
    if (validationResult != RejectionReason::None) {
        return validationResult;
    }
    if (doesOrderExist(order->getOrderID())) {
        return RejectionReason::DuplicateOrderID;
    }
    if (order->getSide() == Side::Buy) {
        return book_side_ops::addToSide(bids, orderIDMap, order);
    } else if (order->getSide() == Side::Sell) {
        return book_side_ops::addToSide(asks, orderIDMap, order);
    }
    return RejectionReason::OrderBookInvariantViolation;
}

RejectionReason LimitOrderBook::cancelOrder(OrderID orderId, OwnerID requesterOwnerID) {
    auto it = orderIDMap.find(orderId);
    if (it == orderIDMap.end())
        return RejectionReason::OrderToBeCancelledDoesNotExist;
    OrderPtr order = *(it->second);
    if (order->getOwnerID() != requesterOwnerID) {
        return RejectionReason::OrderToBeCancelledDoesNotExist;
    }
    RejectionReason validationResult = OrderValidator::validateBeforeAddingOrRemoving(order);
    if (validationResult != RejectionReason::None) {
        return validationResult;
    }
    // TODO: add a book_side_ops free fn for this part
    if (order->getSide() == Side::Buy) {
        auto bookIt = bids.find(order->getPriceTicks());
        if (bookIt != bids.end()) {
            bookIt->second.erase(it->second);
            if (bookIt->second.empty())
                bids.erase(bookIt);
        } else {
            return RejectionReason::OrderBookInvariantViolation;
        }
    } else {
        auto bookIt = asks.find(order->getPriceTicks());
        if (bookIt != asks.end()) {
            bookIt->second.erase(it->second);
            if (bookIt->second.empty())
                asks.erase(bookIt);
        } else {
            return RejectionReason::OrderBookInvariantViolation;
        }
    }
    order->setStatus(OrderLifecycle::afterCancelResting(order->getStatus()));
    orderIDMap.erase(it);
    return RejectionReason::None;
}

bool LimitOrderBook::isOrderMarketable(const OrderPtr &order) const {
    if (order->getType() == OrderType::Cancel) {
        return false;
    }
    if (order->getQty() == 0) {
        return false;
    }
    Side side = order->getSide();
    if (side == Side::Buy && asks.empty()) {
        return false;
    }
    if (side == Side::Sell && bids.empty()) {
        return false;
    }
    if (order->getType() == OrderType::Market) {
        return true;
    }
    if (side == Side::Buy) {
        auto bestAsk = getBestAsk();
        return order->getPriceTicks() >= bestAsk;
    } else {
        auto bestBid = getBestBid();
        return order->getPriceTicks() <= bestBid;
    }
}

bool LimitOrderBook::isFOKFillable(const OrderPtr &order) const {
    if (order->getQty() <= 0) {
        return true;
    }
    if (order->getSide() == Side::Buy) {
        return book_side_ops::isFOKFillableOnSide(asks, order);
    } else {
        return book_side_ops::isFOKFillableOnSide(bids, order);
    }
}

OrderPtr LimitOrderBook::getMatchedOrder(const Side incomingSide) const {
    if (incomingSide == Side::Buy) {
        return book_side_ops::frontOfSide(asks);
    } else {
        return book_side_ops::frontOfSide(bids);
    }
}

void LimitOrderBook::popFront(const Side incomingSide) {
    if (incomingSide == Side::Buy) {
        book_side_ops::popFrontOfSide(asks, orderIDMap);
    } else {
        book_side_ops::popFrontOfSide(bids, orderIDMap);
    }
}

MarketStructureSnapshot LimitOrderBook::snapshot(Timestamp now, std::size_t depthLimit) const {
    MarketStructureSnapshot snap{};
    snap.timestamp = now;
    snap.bestBid = getBestBid();
    snap.bestAsk = getBestAsk();
    snap.spread = getSpread();
    snap.mid = getMidPrice();

    book_side_ops::summariseSide(bids, depthLimit, snap.bidSummary, snap.bidDepths);
    book_side_ops::summariseSide(asks, depthLimit, snap.askSummary, snap.askDepths);

    snap.tempo.tradeExecutionCount = tradeExecutionCount;
    snap.tempo.orderCancellationCount = orderCancellationCount;
    snap.tempo.totalVolumeTraded = totalVolumeTraded;
    return snap;
}
