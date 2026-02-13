#pragma once
#include <algorithm>
#include "models/order.hpp"
#include "models/trade.hpp"
#include "infra/trade_id_generator.hpp"
#include "infra/trade_logger.hpp"

class ExecutionEngine {
public:
    static Quantity executeTrade(const OrderPtr& taker, const OrderPtr& maker) {
        Quantity tradedQty = std::min(taker->getQty(), maker->getQty());
        taker->reduceQty(tradedQty);
        maker->reduceQty(tradedQty);
        return tradedQty;
    }

    static Quantity executeTrade(
        const OrderPtr& taker,
        const OrderPtr& maker,
        TradeLogger* tradeLogger,
        TradeIdGenerator* tradeIdGenerator
    ) {
        Quantity tradedQty = executeTrade(taker, maker);
        if (tradedQty == 0 || tradeLogger == nullptr || tradeIdGenerator == nullptr) {
            return tradedQty;
        }
        Trade trade(
            tradeIdGenerator->nextId(),
            taker->getOrderID(),
            maker->getOrderID(),
            maker->getPriceTicks(),
            tradedQty,
            taker->getSide(),
            taker->getTimestamp()
        );
        tradeLogger->log(trade);
        return tradedQty;
    }
};
