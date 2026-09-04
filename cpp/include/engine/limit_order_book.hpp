#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include "engine/book_side_ops.hpp"
#include "models/market_structure_snapshot.hpp"
#include "models/rejection_reason.hpp"
#include "policy/order_validator.hpp"
#include "policy/order_lifecycle.hpp"

class LimitOrderBook {
private:
    BidStructure bids;
    AskStructure asks;
    OrderIDMap orderIDMap;
    Count tradeExecutionCount = 0;
    Count orderCancellationCount = 0;
    AggregateQuantity totalVolumeTraded = 0;

public:
    LimitOrderBook() = default;

    bool doesOrderExist(OrderID orderId) const;

    std::optional<PriceTicks> getBestBid()  const;
    std::optional<PriceTicks> getBestAsk()  const;
    std::optional<PriceTicks> getMidPrice() const;
    std::optional<PriceTicks> getSpread()   const;

    Count getTradeExecutionCount()           const;
    Count getOrderCancellationCount()        const;
    AggregateQuantity getTotalVolumeTraded() const;

    void recordExecution(Quantity tradedQty);
    void recordCancellation();

    RejectionReason addOrder(const OrderPtr &order);
    RejectionReason cancelOrder(OrderID orderId, OwnerID requesterOwnerID);

    bool isOrderMarketable(const OrderPtr &order)                      const;
    bool isFOKFillable(const OrderPtr &order, STPDecision stpDecision) const;
    
    OrderPtr getMatchedOrder(const Side incomingSide) const;
    
    void popFront(const Side incomingSide);

    MarketStructureSnapshot snapshot(Timestamp now, std::size_t depthLimit = 5) const;
};