#include "engine/limit_order_book.hpp"
#include <iterator>

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

uint32_t LimitOrderBook::getTradeExecutionCount()    const { return tradeExecutionCount; }
uint32_t LimitOrderBook::getOrderCancellationCount() const { return orderCancellationCount; }
uint64_t LimitOrderBook::getTotalVolumeTraded()      const { return totalVolumeTraded; }

void LimitOrderBook::recordExecution(Quantity tradedQty) {
    if (tradedQty > 0) {
        ++tradeExecutionCount;
        totalVolumeTraded += tradedQty;
    }
}

void LimitOrderBook::recordCancellation() {
    ++orderCancellationCount;
}

RejectionReason LimitOrderBook::addOrder(const OrderPtr &order) {
    RejectionReason validationResult = OrderValidator::validateBeforeAdding(order);
    if (validationResult != RejectionReason::None) {
        return validationResult;
    }
    OrderID orderID = order->getOrderID();
    if (doesOrderExist(orderID)) {
        return RejectionReason::DuplicateOrderID;
    }
    PriceTicks price = order->getPriceTicks();
    Side side = order->getSide();
    if (side == Side::Buy) {
        bids[price].push_back(order);
        auto it = std::prev(bids[price].end());
        orderIDMap.emplace(orderID, it);
    } else if (side == Side::Sell) {
        asks[price].push_back(order);
        auto it = std::prev(asks[price].end());
        orderIDMap.emplace(orderID, it);
    } else {
        return RejectionReason::OrderBookInvariantViolation;
    }
    return RejectionReason::None;
}

RejectionReason LimitOrderBook::cancelOrder(OrderID orderId, OwnerID requesterOwnerID) {
    auto it = orderIDMap.find(orderId);
    if (it == orderIDMap.end())
        return RejectionReason::OrderToBeCancelledDoesNotExist;
    OrderPtr order = *(it->second);
    if (order->getOwnerID() != requesterOwnerID) {
        return RejectionReason::OrderToBeCancelledDoesNotExist;
    }
    RejectionReason validationResult = OrderValidator::validateBeforeCancelling(order);
    if (validationResult != RejectionReason::None) {
        return validationResult;
    }
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
    Quantity needed = order->getQty();
    if (needed <= 0) {
        return true;
    }
    OwnerID ownerID = order->getOwnerID();
    bool isMarket = order->getType() == OrderType::Market;
    Quantity available = 0;

    if (order->getSide() == Side::Buy) {
        for (const auto& [price, orders] : asks) {
            if (!isMarket && price > order->getPriceTicks()) {
                break;
            }
            for (const auto& resting : orders) {
                if (resting->getOwnerID() == ownerID) {
                    return false;
                }
                available += resting->getQty();
                if (available >= needed) {
                    return true;
                }
            }
        }
    } else {
        for (const auto& [price, orders] : bids) {
            if (!isMarket && price < order->getPriceTicks()) {
                break;
            }
            for (const auto& resting : orders) {
                if (resting->getOwnerID() == ownerID) {
                    return false;
                }
                available += resting->getQty();
                if (available >= needed) {
                    return true;
                }
            }
        }
    }
    return false;
}

OrderPtr LimitOrderBook::getMatchedOrder(const Side incomingSide) const {
    if (incomingSide == Side::Buy) {
        if (asks.empty() || asks.begin()->second.empty()) return nullptr;
        return asks.begin()->second.front();
    } else {
        if (bids.empty() || bids.begin()->second.empty()) return nullptr;
        return bids.begin()->second.front();
    }
}

void LimitOrderBook::popFront(const Side incomingSide) {
    if (incomingSide == Side::Buy) {
        if (!asks.empty()) {
            auto& askList = asks.begin()->second;
            auto bestAskOrder = askList.front();
            orderIDMap.erase(bestAskOrder->getOrderID());
            askList.pop_front();
            if (askList.empty()) {
                asks.erase(asks.begin());
            }
        }
    } else {
        if (!bids.empty()) {
            auto& bidList = bids.begin()->second;
            auto bestBidOrder = bidList.front();
            orderIDMap.erase(bestBidOrder->getOrderID());
            bidList.pop_front();
            if (bidList.empty()) {
                bids.erase(bids.begin());
            }
        }
    }
}

MarketStructureSnapshot LimitOrderBook::snapshot(Timestamp now, std::size_t depthLimit) const {
    MarketStructureSnapshot snap{};
    snap.timestamp = now;
    snap.bestBid = getBestBid();
    snap.bestAsk = getBestAsk();
    snap.spread = getSpread();
    snap.mid = getMidPrice();

    snap.bidSummary.totalQuantity = 0;
    snap.bidSummary.orderCount = 0;
    snap.bidSummary.totalNotionalValue = 0;

    snap.askSummary.totalQuantity = 0;
    snap.askSummary.orderCount = 0;
    snap.askSummary.totalNotionalValue = 0;

    snap.bidDepths.clear();
    snap.askDepths.clear();

    std::size_t bidLevels = 0;
    for (const auto& [price, orders] : bids) {
        Quantity levelQty = 0;
        for (const auto& order : orders) {
            levelQty += order->getQty();
        }
        snap.bidSummary.totalQuantity += levelQty;
        snap.bidSummary.orderCount += static_cast<uint32_t>(orders.size());
        snap.bidSummary.totalNotionalValue += static_cast<uint64_t>(price) * static_cast<uint64_t>(levelQty);
        if (bidLevels < depthLimit) {
            snap.bidDepths.push_back(LevelInfo{price, levelQty, static_cast<uint32_t>(orders.size())});
            ++bidLevels;
        }
    }

    std::size_t askLevels = 0;
    for (const auto& [price, orders] : asks) {
        Quantity levelQty = 0;
        for (const auto& order : orders) {
            levelQty += order->getQty();
        }
        snap.askSummary.totalQuantity += levelQty;
        snap.askSummary.orderCount += static_cast<uint32_t>(orders.size());
        snap.askSummary.totalNotionalValue += static_cast<uint64_t>(price) * static_cast<uint64_t>(levelQty);
        if (askLevels < depthLimit) {
            snap.askDepths.push_back(LevelInfo{price, levelQty, static_cast<uint32_t>(orders.size())});
            ++askLevels;
        }
    }

    snap.tempo.tradeExecutionCount = tradeExecutionCount;
    snap.tempo.orderCancellationCount = orderCancellationCount;
    snap.tempo.totalVolumeTraded = totalVolumeTraded;
    return snap;
}
