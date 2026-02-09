#pragma once
#include <list>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <expected>
#include "models/order_index.hpp"

enum class OrderError {
    InvalidQuantity,        // qty <= 0
    InvalidPrice,           // priceTicks == 0 for limit orders
    DuplicateOrderId,       // order ID already exists
    NullOrder,              // nullptr passed
    MarketOrderWithPrice,   // market order shouldn't have price constraints
    AddingCancelledOrder,   // trying to add an order that is already cancelled
    AddingExecutedOrder     // trying to add an order that is already executed
};

class LimitOrderBook {
    private:
        std::map<uint64_t, std::list<OrderPtr>, std::greater<uint64_t>> bids;
        std::map<uint64_t, std::list<OrderPtr>> asks;
        std::unordered_map<uint64_t, OrderIndex> orderIndexMap;

    public:
        LimitOrderBook() = default;

        std::expected<void, OrderError> addOrder(const OrderPtr &order) {
            if (!order) {
                return std::unexpected(OrderError::NullOrder);
            }
            
            if (order->getQty() <= 0) {
                return std::unexpected(OrderError::InvalidQuantity);
            }
            
            if (order->getType() == OrderType::Limit && order->getPriceTicks() <= 0) {
                return std::unexpected(OrderError::InvalidPrice);
            }

            if (order->getType() == OrderType::Market && order->getPriceTicks() != 0) {
                return std::unexpected(OrderError::MarketOrderWithPrice);
            }

            if (order->isCancelled()) {
                return std::unexpected(OrderError::AddingCancelledOrder);
            }

            if (order->getStatus() == OrderStatus::Executed) {
                return std::unexpected(OrderError::AddingExecutedOrder);
            }
            
            if (orderIndexMap.contains(order->getOrderID())) {
                return std::unexpected(OrderError::DuplicateOrderId);
            }

            uint64_t price = order->getPriceTicks();
            if (order->getSide() == Side::Buy) {
                bids[price].push_back(order);
                auto it = std::prev(bids[price].end());
                orderIndexMap.emplace(order->getOrderID(), OrderIndex(Side::Buy, price, it));
            } else {
                asks[price].push_back(order);
                auto it = std::prev(asks[price].end());
                orderIndexMap.emplace(order->getOrderID(), OrderIndex(Side::Sell, price, it));
            }
            return {};
        }

        bool removeOrder(uint64_t orderId) {
            auto it = orderIndexMap.find(orderId);
            if (it == orderIndexMap.end())
                return false;
            OrderIndex& idx = it->second;
            if (idx.getSide() == Side::Buy) {
                auto bookIt = bids.find(idx.getPriceTicks());
                if (bookIt != bids.end()) {
                    bookIt->second.erase(idx.getOrderIter());
                    if (bookIt->second.empty())
                        bids.erase(bookIt);
                }
                else {
                    return false;
                }
            } else {
                auto bookIt = asks.find(idx.getPriceTicks());
                if (bookIt != asks.end()) {
                    bookIt->second.erase(idx.getOrderIter());
                    if (bookIt->second.empty())
                        asks.erase(bookIt);
                }
                else {
                    return false;
                }
            }
            orderIndexMap.erase(it);
            return true;
        }

        uint64_t getBestBid() const {
            if (bids.empty()) return 0;
            return bids.begin()->first;
        }

        uint64_t getBestAsk() const {
            if (asks.empty()) return 0;
            return asks.begin()->first;
        }

        bool isOrderMarketable(const OrderPtr &order) const {
            assert(order != nullptr);
            OrderStatus status = order->getStatus();
            assert(status != OrderStatus::Cancelled && status != OrderStatus::CancelledAfterPartialExecution && status != OrderStatus::Executed);
            if (order->getQty() == 0) return false;
            Side side = order->getSide();
            if (side == Side::Buy && asks.empty()) return false;
            if (side == Side::Sell && bids.empty()) return false;
            if (order->getType() == OrderType::Market) return true;
            if (side == Side::Buy) {
                uint64_t bestAsk = getBestAsk();
                return order->getPriceTicks() >= bestAsk;
            } else {
                uint64_t bestBid = getBestBid();
                return order->getPriceTicks() <= bestBid;
            }
        }

        OrderPtr getBestMatchedOrder(const Side side) const {
            if (side == Side::Buy) {
                if (asks.empty() || asks.begin()->second.empty()) return nullptr;
                return asks.begin()->second.front();
            } else {
                if (bids.empty() || bids.begin()->second.empty()) return nullptr;
                return bids.begin()->second.front();
            }
        }

        void popFront(const Side side) {
            if (side == Side::Buy) {
                if (!asks.empty()) {
                    auto& askList = asks.begin()->second;
                    auto bestAskOrder = askList.front();
                    orderIndexMap.erase(bestAskOrder->getOrderID());
                    askList.pop_front();
                    if (askList.empty()) {
                        asks.erase(asks.begin());
                    }
                }
            } else {
                if (!bids.empty()) {
                    auto& bidList = bids.begin()->second;
                    auto bestBidOrder = bidList.front();
                    orderIndexMap.erase(bestBidOrder->getOrderID());
                    bidList.pop_front();
                    if (bidList.empty()) {
                        bids.erase(bids.begin());
                    }
                }
            }
        }
};