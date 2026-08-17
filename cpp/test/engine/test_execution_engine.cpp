#include <gtest/gtest.h>
#include <vector>
#include "infra/trade_id_generator.hpp"
#include "infra/trade_logger.hpp"
#include "engine/execution_engine.hpp"
#include "models/order.hpp"
#include "models/trade.hpp"

class MockTradeLogger : public TradeLogger {
public:
    std::vector<Trade> trades;

    void log(const Trade& trade) override {
        trades.push_back(trade);
    }

    void flush() override {}
    void close() override {}
};

class MockTradeIdGenerator : public TradeIdGenerator {
public:
    explicit MockTradeIdGenerator(TradeID id) : fixedId(id) {}

    TradeID nextId() override {
        return fixedId;
    }

private:
    TradeID fixedId;
};

class ExecutionEngineTest : public ::testing::Test {
protected:
    OrderPtr buyOrder;
    OrderPtr sellOrder;

    void SetUp() override {
        buyOrder = std::make_shared<Order>(1, 100, 1000, 100, Side::Buy, OrderType::Market, 1622547800);
        sellOrder = std::make_shared<Order>(2, 200, 1000, 100, Side::Sell, OrderType::Limit, 1622547801);
    }
};

TEST_F(ExecutionEngineTest, ExecuteTradeWithEqualQuantities) {
    uint32_t tradedQty = ExecutionEngine::executeTrade(buyOrder, sellOrder);
    
    EXPECT_EQ(tradedQty, 100u);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_EQ(sellOrder->getQty(), 0);
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithTakerSmallerQuantity) {
    OrderPtr largerSellOrder = std::make_shared<Order>(3, 300, 1000, 150, Side::Sell, OrderType::Limit, 1622547802);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(buyOrder, largerSellOrder);
    
    EXPECT_EQ(tradedQty, 100u);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_EQ(largerSellOrder->getQty(), 50);
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithMakerSmallerQuantity) {
    OrderPtr largerBuyOrder = std::make_shared<Order>(4, 400, 1000, 150, Side::Buy, OrderType::Limit, 1622547803);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(largerBuyOrder, sellOrder);
    
    EXPECT_EQ(tradedQty, 100u);
    EXPECT_EQ(largerBuyOrder->getQty(), 50);
    EXPECT_EQ(sellOrder->getQty(), 0);
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithZeroQuantities) {
    OrderPtr emptyBuyOrder = std::make_shared<Order>(5, 500, 1000, 0, Side::Buy, OrderType::Limit, 1622547804);
    OrderPtr emptySellOrder = std::make_shared<Order>(6, 600, 1000, 0, Side::Sell, OrderType::Limit, 1622547805);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(emptyBuyOrder, emptySellOrder);
    
    EXPECT_EQ(tradedQty, 0u);
    EXPECT_EQ(emptyBuyOrder->getQty(), 0);
    EXPECT_EQ(emptySellOrder->getQty(), 0);
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithOneZeroQuantity) {
    OrderPtr emptySellOrder = std::make_shared<Order>(7, 700, 1000, 0, Side::Sell, OrderType::Limit, 1622547806);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(buyOrder, emptySellOrder);
    
    EXPECT_EQ(tradedQty, 0u);
    EXPECT_EQ(buyOrder->getQty(), 100);
    EXPECT_EQ(emptySellOrder->getQty(), 0);
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithLargeQuantities) {
    OrderPtr largeBuyOrder = std::make_shared<Order>(10, 1000, 1000, 1000000u, Side::Buy, OrderType::Limit, 1622547809);
    OrderPtr largeSellOrder = std::make_shared<Order>(11, 1100, 1000, 2000000u, Side::Sell, OrderType::Limit, 1622547810);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(largeBuyOrder, largeSellOrder);
    
    EXPECT_EQ(tradedQty, 1000000u);
    EXPECT_EQ(largeBuyOrder->getQty(), 0);
    EXPECT_EQ(largeSellOrder->getQty(), 1000000);
}

TEST_F(ExecutionEngineTest, ExecuteTradeDoesNotAffectOtherOrderFields) {
    uint32_t originalBuyOrderID = buyOrder->getOrderID();
    uint32_t originalBuyOwnerID = buyOrder->getOwnerID();
    uint64_t originalBuyPrice = buyOrder->getPriceTicks();
    uint64_t originalBuyTimestamp = buyOrder->getTimestamp();
    
    ExecutionEngine::executeTrade(buyOrder, sellOrder);
    
    EXPECT_EQ(buyOrder->getOrderID(), originalBuyOrderID);
    EXPECT_EQ(buyOrder->getOwnerID(), originalBuyOwnerID);
    EXPECT_EQ(buyOrder->getPriceTicks(), originalBuyPrice);
    EXPECT_EQ(buyOrder->getTimestamp(), originalBuyTimestamp);
}

TEST_F(ExecutionEngineTest, LogsTradeWhenQtyTraded) {
    MockTradeLogger logger;
    MockTradeIdGenerator idGenerator(9001);

    Quantity tradedQty = ExecutionEngine::executeTrade(buyOrder, sellOrder, &logger, &idGenerator);

    ASSERT_EQ(logger.trades.size(), 1u);
    const Trade& trade = logger.trades.front();
    EXPECT_EQ(trade.getTradeID(), 9001u);
    EXPECT_EQ(trade.getTakerOrderID(), buyOrder->getOrderID());
    EXPECT_EQ(trade.getMakerOrderID(), sellOrder->getOrderID());
    EXPECT_EQ(trade.getPriceTicks(), sellOrder->getPriceTicks());
    EXPECT_EQ(trade.getQty(), tradedQty);
    EXPECT_EQ(trade.getSide(), buyOrder->getSide());
    EXPECT_EQ(trade.getTimestamp(), buyOrder->getTimestamp());
}

TEST_F(ExecutionEngineTest, DoesNotLogWhenTradedQtyZero) {
    MockTradeLogger logger;
    MockTradeIdGenerator idGenerator(9002);
    OrderPtr emptySellOrder = std::make_shared<Order>(7, 700, 1000, 0, Side::Sell, OrderType::Limit, 1622547806);

    Quantity tradedQty = ExecutionEngine::executeTrade(buyOrder, emptySellOrder, &logger, &idGenerator);

    EXPECT_EQ(tradedQty, 0);
    EXPECT_TRUE(logger.trades.empty());
}
