#include <list>
#include <map>
#include <unordered_map>
#include <cstdint>
#include "models/order.hpp"
#include "models/order_index.hpp"

enum class STPProtocol : uint8_t {
    CancelNewest = 0,
    CancelOldest = 1,
    CancelBoth = 2,
};

class LimitOrderBook {
    private:
        std::map<uint64_t, std::list<OrderPtr>, std::greater<uint64_t>> bids;
        std::map<uint64_t, std::list<OrderPtr>> asks;
        std::unordered_map<uint64_t, OrderIndex> orderIndexMap;
        STPProtocol stpProtocol = STPProtocol::CancelBoth;

    public:
        LimitOrderBook() = default;

        LimitOrderBook(STPProtocol stpProtocol_) : stpProtocol(stpProtocol_) {}

        inline STPProtocol getSTPProtocol() const { return stpProtocol; }

        bool addOrder(const OrderPtr &order) {
            if (order->getQty() == 0 || order->getType() == OrderType::Market) return false;
            uint64_t price = order->getPriceTicks();
            if (order->getSide() == Side::Buy) {
                bids[price].push_back(order);
                auto it = std::prev(bids[price].end());
                orderIndexMap.emplace(order->getOrderID(), OrderIndex(1, price, it));
            } else {
                asks[price].push_back(order);
                auto it = std::prev(asks[price].end());
                orderIndexMap.emplace(order->getOrderID(), OrderIndex(0, price, it));
            }
            return true;
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

        uint64_t getBestBid() const {
            if (bids.empty()) return 0;
            return bids.begin()->first;
        }

        uint64_t getBestAsk() const {
            if (asks.empty()) return 0;
            return asks.begin()->first;
        }

        bool isOrderMarketable(const OrderPtr &order) const {
            if (order->getQty() == 0) return false;
            if (order->getType() == OrderType::Market) return true;
            if (order->getSide() == Side::Buy) {
                uint64_t bestAsk = getBestAsk();
                return bestAsk != 0 && order->getPriceTicks() >= bestAsk;
            } else {
                uint64_t bestBid = getBestBid();
                return bestBid != 0 && order->getPriceTicks() <= bestBid;
            }
        }

        bool isSelfTrade(const OrderPtr &order1, const OrderPtr &order2) const {
            return order1->getOwnerID() == order2->getOwnerID();
        }

        void enforceSTP(const OrderPtr &order1, const OrderPtr &order2) {
            const OrderPtr &newest = (order1->getTimestamp() > order2->getTimestamp()) ? order1 : order2;
            const OrderPtr &oldest = (order1->getTimestamp() > order2->getTimestamp()) ? order2 : order1;
            if (stpProtocol == STPProtocol::CancelBoth) {
                cancelOrder(oldest->getOrderID());
                cancelOrder(newest->getOrderID());
            } else if (stpProtocol == STPProtocol::CancelNewest) {
                cancelOrder(newest->getOrderID());
            } else if (stpProtocol == STPProtocol::CancelOldest) {
                cancelOrder(oldest->getOrderID());
            }
        }

        void matchOrder(const OrderPtr &order) {
            if(order->getSide() == Side::Buy) {
                while (isOrderMarketable(order) && asks.size()) {
                    auto& askList = asks.begin()->second;
                    auto bestAskOrder = askList.front();
                    if (isSelfTrade(order, bestAskOrder)) {
                        enforceSTP(order, bestAskOrder);
                        continue;
                    }
                    uint32_t tradeQty = std::min(order->getQty(), bestAskOrder->getQty());
                    order->reduceQty(tradeQty);
                    bestAskOrder->reduceQty(tradeQty);
                    if (bestAskOrder->getQty() == 0) {
                        askList.pop_front();
                        orderIndexMap.erase(bestAskOrder->getOrderID());
                        if (askList.empty()) {
                            asks.erase(asks.begin());
                        }
                    }
                }
            }
            else {
                while (isOrderMarketable(order) && bids.size()) {
                    auto& bidList = bids.begin()->second;
                    auto bestBidOrder = bidList.front();
                    if (isSelfTrade(order, bestBidOrder)) {
                        enforceSTP(order, bestBidOrder);
                        continue;
                    }
                    uint32_t tradeQty = std::min(order->getQty(), bestBidOrder->getQty());
                    order->reduceQty(tradeQty);
                    bestBidOrder->reduceQty(tradeQty);
                    if (bestBidOrder->getQty() == 0) {
                        bidList.pop_front();
                        orderIndexMap.erase(bestBidOrder->getOrderID());
                        if (bidList.empty()) {
                            bids.erase(bids.begin());
                        }
                    }
                }
            }
            if (order->getQty() > 0 && order->getType() == OrderType::Limit) {
                addOrder(order);
            }
        }

};