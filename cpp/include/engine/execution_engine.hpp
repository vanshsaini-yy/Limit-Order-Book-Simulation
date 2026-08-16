#pragma once
#include <algorithm>
#include "models/order.hpp"
#include "models/trade.hpp"
#include "infra/trade_id_generator.hpp"
#include "infra/trade_logger.hpp"

class ExecutionEngine {
public:
    static Quantity executeTrade(const OrderPtr& taker, const OrderPtr& maker);

    static Quantity executeTrade(
        const OrderPtr& taker,
        const OrderPtr& maker,
        TradeLogger* tradeLogger,
        TradeIdGenerator* tradeIdGenerator
    );
};
