#pragma once

#include <list>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <cstddef>
#include <expected>
#include "models/market_structure_snapshot.hpp"
#include "policy/order_validation.hpp"
#include "policy/order_lifecycle.hpp"

using BidStructure = std::map<PriceTicks, std::list<OrderPtr>, std::greater<PriceTicks>>;
using AskStructure = std::map<PriceTicks, std::list<OrderPtr>>;

class LimitOrderBook {
    private:
        BidStructure bids;
        AskStructure asks;
        std::unordered_map<OrderID, std::list<OrderPtr>::iterator> orderIDMap;
        uint32_t executionCount = 0;
        uint32_t cancelCount = 0;
        uint64_t totalVolumeExecuted = 0;

    public:
        LimitOrderBook() = default;

        MarketStructureSnapshot snapshot(Timestamp now, std::size_t depthLimit = 5) const {
            MarketStructureSnapshot snapshot{};
            snapshot.timestamp = now;
            snapshot.bestBid = getBestBid();
            snapshot.bestAsk = getBestAsk();
            if (snapshot.bestBid.has_value() && snapshot.bestAsk.has_value()) {
                snapshot.spread = snapshot.bestAsk.value() - snapshot.bestBid.value();
                snapshot.mid = (snapshot.bestAsk.value() + snapshot.bestBid.value()) / 2;
            }
            
            snapshot.bidSummary.totalQuantity = 0;
            snapshot.bidSummary.orderCount = 0;
            snapshot.bidSummary.totalNotionalValue = 0;

            snapshot.askSummary.totalQuantity = 0;
            snapshot.askSummary.orderCount = 0;
            snapshot.askSummary.totalNotionalValue = 0;

            snapshot.bidDepths.clear();
            snapshot.askDepths.clear();

            std::size_t bidLevels = 0;
            for (const auto& [price, orders] : bids) {
                Quantity levelQty = 0;
                for (const auto& order : orders) {
                    levelQty += order->getQty();
                }
                snapshot.bidSummary.totalQuantity += levelQty;
                snapshot.bidSummary.orderCount += static_cast<uint32_t>(orders.size());
                snapshot.bidSummary.totalNotionalValue += static_cast<uint64_t>(price) * static_cast<uint64_t>(levelQty);
                if (bidLevels < depthLimit) {
                    snapshot.bidDepths.push_back(LevelInfo{price, levelQty, static_cast<uint32_t>(orders.size())});
                    ++bidLevels;
                }
            }

            std::size_t askLevels = 0;
            for (const auto& [price, orders] : asks) {
                Quantity levelQty = 0;
                for (const auto& order : orders) {
                    levelQty += order->getQty();
                }
                snapshot.askSummary.totalQuantity += levelQty;
                snapshot.askSummary.orderCount += static_cast<uint32_t>(orders.size());
                snapshot.askSummary.totalNotionalValue += static_cast<uint64_t>(price) * static_cast<uint64_t>(levelQty);
                if (askLevels < depthLimit) {
                    snapshot.askDepths.push_back(LevelInfo{price, levelQty, static_cast<uint32_t>(orders.size())});
                    ++askLevels;
                }
            }

            snapshot.tempo.executionCount = executionCount;
            snapshot.tempo.cancelCount = cancelCount;
            snapshot.tempo.totalVolumeExecuted = totalVolumeExecuted;
            return snapshot;
        }

        void recordExecution(Quantity qtyExecuted) {
            ++executionCount;
            totalVolumeExecuted += qtyExecuted;
        }

        void recordCancellation() {
            ++cancelCount;
        }

        bool doesOrderExist(OrderID orderId) const {
            return orderIDMap.contains(orderId);
        }

        std::optional<PriceTicks> getBestBid() const {
            if (bids.empty()) return std::nullopt;
            return bids.begin()->first;
        }

        std::optional<PriceTicks> getBestAsk() const {
            if (asks.empty()) return std::nullopt;
            return asks.begin()->first;
        }

        RejectionReason addOrder(const OrderPtr &order) {
            RejectionReason validationResult = OrderValidator::validateBeforeAdding(order);
            if (validationResult != RejectionReason::None) {
                return validationResult;
            }
            OrderID orderID = order->getOrderID();
            if (doesOrderExist(orderID)) {
                return RejectionReason::AddingDuplicateOrder;
            }
            PriceTicks price = order->getPriceTicks();
            if (order->getSide() == Side::Buy) {
                bids[price].push_back(order);
                auto it = std::prev(bids[price].end());
                orderIDMap.emplace(orderID, it);
            } else {
                asks[price].push_back(order);
                auto it = std::prev(asks[price].end());
                orderIDMap.emplace(orderID, it);
            }
            return RejectionReason::None;
        }

        RejectionReason cancelOrder(OrderID orderId) {
            auto it = orderIDMap.find(orderId);
            if (it == orderIDMap.end())
                return RejectionReason::OrderToBeCancelledDoesNotExist;
            OrderPtr order = *(it->second);
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
                }
                else {
                    return RejectionReason::OrderBookInvariantViolation;
                }
            } else {
                auto bookIt = asks.find(order->getPriceTicks());
                if (bookIt != asks.end()) {
                    bookIt->second.erase(it->second);
                    if (bookIt->second.empty())
                        asks.erase(bookIt);
                }
                else {
                    return RejectionReason::OrderBookInvariantViolation;
                }
            }
            order->setStatus(OrderLifecycle::afterCancelResting(order->getStatus()));
            orderIDMap.erase(it);
            return RejectionReason::None;
        }

        bool isOrderMarketable(const OrderPtr &order) const {
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

        OrderPtr getMatchedOrder(const Side incomingSide) const {
            if (incomingSide == Side::Buy) {
                if (asks.empty() || asks.begin()->second.empty()) return nullptr;
                return asks.begin()->second.front();
            } else {
                if (bids.empty() || bids.begin()->second.empty()) return nullptr;
                return bids.begin()->second.front();
            }
        }

        void popFront(const Side incomingSide) {
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
};