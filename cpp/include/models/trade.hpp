#pragma once
#include <cstdint>
#include "models/order.hpp"

// TODO: make TradeID uint32_t
using TradeID = uint64_t;

class Trade {
private:
    TradeID tradeID;
    OrderID takerOrderID;
    OrderID makerOrderID;
    PriceTicks priceTicks;
    Quantity qty;
    Side side;
    Timestamp timestamp;

public:
    Trade(
        TradeID tradeID_,
        OrderID takerOrderID_,
        OrderID makerOrderID_,
        PriceTicks priceTicks_,
        Quantity qty_,
        Side side_,
        Timestamp timestamp_
    );

    TradeID    getTradeID()      const;
    Timestamp  getTimestamp()    const;
    PriceTicks getPriceTicks()   const;
    Quantity   getQty()          const;
    Side       getSide()         const;
    OrderID    getTakerOrderID() const;
    OrderID    getMakerOrderID() const;
};