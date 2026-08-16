#pragma once

#include <list>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <cstddef>
#include <optional>
#include "models/market_structure_snapshot.hpp"
#include "models/rejection_reason.hpp"
#include "policy/order_validator.hpp"
#include "policy/order_lifecycle.hpp"

using BidStructure = std::map<PriceTicks, std::list<OrderPtr>, std::greater<PriceTicks>>;
using AskStructure = std::map<PriceTicks, std::list<OrderPtr>>;

class LimitOrderBook {
    private:
        BidStructure bids;
        AskStructure asks;
        std::unordered_map<OrderID, std::list<OrderPtr>::iterator> orderIDMap;
        uint32_t tradeExecutionCount = 0;
        uint32_t orderCancellationCount = 0;
        uint64_t totalVolumeTraded = 0;

    public:
        LimitOrderBook() = default;

        bool doesOrderExist(OrderID orderId) const;

        std::optional<PriceTicks> getBestBid()  const;
        std::optional<PriceTicks> getBestAsk()  const;
        std::optional<PriceTicks> getMidPrice() const;
        std::optional<PriceTicks> getSpread()   const;
        
        uint32_t getTradeExecutionCount()    const;
        uint32_t getOrderCancellationCount() const;
        uint64_t getTotalVolumeTraded()      const;

        void recordExecution(Quantity tradedQty);
        void recordCancellation();

        RejectionReason addOrder(const OrderPtr &order);
        RejectionReason cancelOrder(OrderID orderId, OwnerID requesterOwnerID);

        bool     isOrderMarketable(const OrderPtr &order) const;
        bool     isFOKFillable(const OrderPtr &order) const;
        OrderPtr getMatchedOrder(const Side incomingSide) const;
        void     popFront(const Side incomingSide);

        MarketStructureSnapshot snapshot(Timestamp now, std::size_t depthLimit = 5) const;
};