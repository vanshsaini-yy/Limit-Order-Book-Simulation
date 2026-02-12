#pragma once
#include <list>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <expected>
#include "policy/order_validation.hpp"

using BidStructure = std::map<PriceTicks, std::list<OrderPtr>, std::greater<PriceTicks>>;
using AskStructure = std::map<PriceTicks, std::list<OrderPtr>>;

class LimitOrderBook {
    private:
        BidStructure bids;
        AskStructure asks;
        std::unordered_map<OrderID, std::list<OrderPtr>::iterator> orderIDMap;

    public:
        LimitOrderBook() = default;

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

        RejectionReason removeOrder(OrderID orderId) {
            auto it = orderIDMap.find(orderId);
            if (it == orderIDMap.end())
                return RejectionReason::OrderToBeRemovedDoesNotExist;
            OrderPtr order = *(it->second);
            RejectionReason validationResult = OrderValidator::validateBeforeRemoving(order);
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
            orderIDMap.erase(it);
            return RejectionReason::None;
        }

        bool isOrderMarketable(const OrderPtr &order) const {
            if (order->getQty() == 0) return false;
            Side side = order->getSide();
            if (side == Side::Buy && asks.empty()) return false;
            if (side == Side::Sell && bids.empty()) return false;
            if (order->getType() == OrderType::Market) return true;
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