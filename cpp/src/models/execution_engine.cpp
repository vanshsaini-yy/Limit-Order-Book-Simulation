#include "models/execution_engine.hpp"

Quantity ExecutionEngine::executeTrade(const OrderPtr& taker, const OrderPtr& maker) {
    Quantity tradedQty = std::min(taker->getQty(), maker->getQty());
    taker->reduceQty(tradedQty);
    maker->reduceQty(tradedQty);
    return tradedQty;
}

Quantity ExecutionEngine::executeTrade(
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
