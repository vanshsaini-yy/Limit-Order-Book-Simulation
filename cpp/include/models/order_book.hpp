#include <list>
#include <map>
#include <cstdint>
#include "models/order.hpp"
#include "models/order_index.hpp"


class LimitOrderBook{
    private:
        std::map<uint64_t, std::list<std::shared_ptr<Order>>, std::greater<uint64_t>> bids;
        std::map<uint64_t, std::list<std::shared_ptr<Order>>> asks;
        std::unordered_map<uint64_t, OrderIndex> orderIndexMap;
    public:
        void addOrder(const std::shared_ptr<Order>& order) {
            uint64_t price = order->getPriceTicks();
            if (order->getSide() == Side::Buy) {
                bids[price].push_back(order);
                auto it = std::prev(bids[price].end());
                orderIndexMap.emplace(order->getId(), OrderIndex(1, price, it));
            } else {
                asks[price].push_back(order);
                auto it = std::prev(asks[price].end());
                orderIndexMap.emplace(order->getId(), OrderIndex(0, price, it));
            }
        }

        bool cancelOrder(uint64_t orderId) {
            auto it = orderIndexMap.find(orderId);
            if (it == orderIndexMap.end())
                return false;
            OrderIndex& idx = it->second;
            if (idx.getIsBuy()) {
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
};