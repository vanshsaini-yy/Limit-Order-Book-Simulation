#include <gtest/gtest.h>
#include "models/execution_engine.hpp"
#include "models/order.hpp"

class ExecutionEngineTest : public ::testing::Test {
protected:
    Order* buyOrder;
    Order* sellOrder;

    void SetUp() override {
        // Create a buy order with quantity 100 at price 1000
        buyOrder = new Order(1, 100, 1000, 100, Side::Buy, OrderType::Limit, 1622547800);
        // Create a sell order with quantity 100 at price 1000
        sellOrder = new Order(2, 200, 1000, 100, Side::Sell, OrderType::Limit, 1622547801);
    }

    void TearDown() override {
        delete buyOrder;
        delete sellOrder;
    }
};

TEST_F(ExecutionEngineTest, ExecuteTradeWithEqualQuantities) {
    uint32_t tradedQty = ExecutionEngine::executeTrade(buyOrder, sellOrder);
    
    // Both orders should be completely filled
    EXPECT_EQ(tradedQty, 100u);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getQty(), 0u);
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithTakerSmallerQuantity) {
    // Create a sell order with larger quantity
    Order* largerSellOrder = new Order(3, 300, 1000, 150, Side::Sell, OrderType::Limit, 1622547802);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(buyOrder, largerSellOrder);
    
    // Trade should be for the taker's quantity
    EXPECT_EQ(tradedQty, 100u);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(largerSellOrder->getQty(), 50u);
    
    delete largerSellOrder;
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithMakerSmallerQuantity) {
    // Create a buy order with larger quantity
    Order* largerBuyOrder = new Order(4, 400, 1000, 150, Side::Buy, OrderType::Limit, 1622547803);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(largerBuyOrder, sellOrder);
    
    // Trade should be for the maker's quantity
    EXPECT_EQ(tradedQty, 100u);
    EXPECT_EQ(largerBuyOrder->getQty(), 50u);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    
    delete largerBuyOrder;
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithZeroQuantities) {
    // Create orders with zero quantity
    Order* emptyBuyOrder = new Order(5, 500, 1000, 0, Side::Buy, OrderType::Limit, 1622547804);
    Order* emptySellOrder = new Order(6, 600, 1000, 0, Side::Sell, OrderType::Limit, 1622547805);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(emptyBuyOrder, emptySellOrder);
    
    EXPECT_EQ(tradedQty, 0u);
    EXPECT_EQ(emptyBuyOrder->getQty(), 0u);
    EXPECT_EQ(emptySellOrder->getQty(), 0u);
    
    delete emptyBuyOrder;
    delete emptySellOrder;
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithOneZeroQuantity) {
    // Create a sell order with zero quantity
    Order* emptySellOrder = new Order(7, 700, 1000, 0, Side::Sell, OrderType::Limit, 1622547806);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(buyOrder, emptySellOrder);
    
    EXPECT_EQ(tradedQty, 0u);
    EXPECT_EQ(buyOrder->getQty(), 100u);
    EXPECT_EQ(emptySellOrder->getQty(), 0u);
    
    delete emptySellOrder;
}

TEST_F(ExecutionEngineTest, ExecuteTradeWithLargeQuantities) {
    // Create orders with large quantities
    Order* largeBuyOrder = new Order(10, 1000, 1000, 1000000u, Side::Buy, OrderType::Limit, 1622547809);
    Order* largeSellOrder = new Order(11, 1100, 1000, 2000000u, Side::Sell, OrderType::Limit, 1622547810);
    
    uint32_t tradedQty = ExecutionEngine::executeTrade(largeBuyOrder, largeSellOrder);
    
    EXPECT_EQ(tradedQty, 1000000u);
    EXPECT_EQ(largeBuyOrder->getQty(), 0u);
    EXPECT_EQ(largeSellOrder->getQty(), 1000000u);
    
    delete largeBuyOrder;
    delete largeSellOrder;
}

TEST_F(ExecutionEngineTest, ExecuteTradeDoesNotAffectOtherOrderFields) {
    uint32_t originalBuyOrderID = buyOrder->getOrderID();
    uint32_t originalBuyOwnerID = buyOrder->getOwnerID();
    uint64_t originalBuyPrice = buyOrder->getPriceTicks();
    uint64_t originalBuyTimestamp = buyOrder->getTimestamp();
    
    ExecutionEngine::executeTrade(buyOrder, sellOrder);
    
    // Verify that other fields remain unchanged
    EXPECT_EQ(buyOrder->getOrderID(), originalBuyOrderID);
    EXPECT_EQ(buyOrder->getOwnerID(), originalBuyOwnerID);
    EXPECT_EQ(buyOrder->getPriceTicks(), originalBuyPrice);
    EXPECT_EQ(buyOrder->getTimestamp(), originalBuyTimestamp);
}
