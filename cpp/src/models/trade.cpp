#include "models/trade.hpp"

Trade::Trade(
    TradeID tradeID_,
    OrderID takerOrderID_,
    OrderID makerOrderID_,
    PriceTicks priceTicks_,
    Quantity qty_,
    Side side_,
    Timestamp timestamp_
)
:   tradeID(tradeID_),
    takerOrderID(takerOrderID_),
    makerOrderID(makerOrderID_),
    priceTicks(priceTicks_),
    qty(qty_),
    side(side_),
    timestamp(timestamp_) {}

TradeID    Trade::getTradeID()      const { return tradeID; }
Timestamp  Trade::getTimestamp()    const { return timestamp; }
PriceTicks Trade::getPriceTicks()   const { return priceTicks; }
Quantity   Trade::getQty()          const { return qty; }
Side       Trade::getSide()         const { return side; }
OrderID    Trade::getTakerOrderID() const { return takerOrderID; }
OrderID    Trade::getMakerOrderID() const { return makerOrderID; }
