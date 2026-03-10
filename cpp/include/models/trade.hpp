#pragma once
#include <cstdint>
#include "models/order.hpp"

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
        )
        :   tradeID(tradeID_),
            takerOrderID(takerOrderID_),
            makerOrderID(makerOrderID_),
            priceTicks(priceTicks_),
            qty(qty_),
            side(side_),
            timestamp(timestamp_) {}

        inline TradeID getTradeID() const { return tradeID; }
        inline Timestamp getTimestamp() const { return timestamp; }
        inline PriceTicks getPriceTicks() const { return priceTicks; }
        inline Quantity getQty() const { return qty; }
        inline Side getSide() const { return side; }
        inline OrderID getTakerOrderID() const { return takerOrderID; }
        inline OrderID getMakerOrderID() const { return makerOrderID; }
};
