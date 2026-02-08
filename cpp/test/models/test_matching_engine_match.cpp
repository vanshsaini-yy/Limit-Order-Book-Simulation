#include <gtest/gtest.h>
#include <memory>
#include "models/matching_engine.hpp"

class MatchingEngineTest : public ::testing::Test {
protected:
    LimitOrderBook* orderBook;
    STPPolicy* stpPolicy;
    MatchingEngine* engine;

    void SetUp() override {
        stpPolicy = new CancelBothSTP();
        orderBook = new LimitOrderBook();
        engine = new MatchingEngine(stpPolicy, orderBook);
    }

    void TearDown() override {
        delete engine;
        delete orderBook;
        delete stpPolicy;
    }
};

TEST_F(MatchingEngineTest, MatchLimitBuyToEmptyBook) {
    OrderPtr buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);

    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder;
}

TEST_F(MatchingEngineTest, MatchLimitSellToEmptyBook) {
    OrderPtr sellOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);

    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);

    delete sellOrder;
}
TEST_F(MatchingEngineTest, LimitBuyOrderExactMatch) {
    OrderPtr sellOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = new Order(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801);
    
    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete sellOrder;
    delete buyOrder;
}


TEST_F(MatchingEngineTest, LimitSellOrderExactMatch) {
    OrderPtr buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, LimitBuyOrderMatchesBestAsk) {
    OrderPtr sellOrder1 = new Order(1, 1, 103, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = new Order(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = new Order(3, 3, 105, 10, Side::Buy, OrderType::Limit, 1622547802);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder1->getQty(), 10u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder2->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 103u);

    delete sellOrder1;
    delete sellOrder2;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, LimitSellOrderMatchesBestBid) {
    OrderPtr buyOrder1 = new Order(1, 1, 97, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = new Order(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = new Order(3, 3, 95, 10, Side::Sell, OrderType::Limit, 1622547802);

    engine->matchOrder(buyOrder1);
    engine->matchOrder(buyOrder2);
    engine->matchOrder(sellOrder);

    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder1->getQty(), 10u);
    EXPECT_EQ(buyOrder2->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder2->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 97u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder1;
    delete buyOrder2;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, RestingBuyOrderPartialFill_IncomingLimitSell) {
    OrderPtr buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = new Order(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder->getQty(), 5u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, RestingSellOrderPartialFill_IncomingLimitBuy) {
    OrderPtr sellOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = new Order(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);

    delete sellOrder;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, IncomingLimitBuyOrderPartialFill) {
    OrderPtr sellOrder = new Order(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = new Order(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder->getQty(), 5u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, IncomingLimitSellOrderPartialFill) {
    OrderPtr buyOrder = new Order(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder->getQty(), 5u);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);

    delete buyOrder;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, LimitBuyOrderSweepsMultipleLevels) {
    OrderPtr sellOrder1 = new Order(2, 2, 100, 50, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr sellOrder2 = new Order(3, 3, 102, 10, Side::Sell, OrderType::Limit, 1622547802);
    OrderPtr buyOrder = new Order(1, 1, 103, 55, Side::Buy, OrderType::Limit, 1622547803);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder1->getQty(), 0u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 102u);

    delete sellOrder1;
    delete sellOrder2;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, LimitSellOrderSweepsMultipleLevels) {
    OrderPtr buyOrder1 = new Order(2, 2, 100, 50, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr buyOrder2 = new Order(3, 3, 98, 10, Side::Buy, OrderType::Limit, 1622547802);
    OrderPtr sellOrder = new Order(1, 1, 97, 55, Side::Sell, OrderType::Limit, 1622547803);

    engine->matchOrder(buyOrder1);
    engine->matchOrder(buyOrder2);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder1->getQty(), 0u);
    EXPECT_EQ(buyOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 98u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder1;
    delete buyOrder2;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, LimitBuyOrderSweepsMultipleOrdersSameLevel) {
    OrderPtr sellOrder1 = new Order(2, 2, 100, 50, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr sellOrder2 = new Order(3, 3, 100, 10, Side::Sell, OrderType::Limit, 1622547802);
    OrderPtr buyOrder = new Order(1, 1, 100, 55, Side::Buy, OrderType::Limit, 1622547803);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder1->getQty(), 0u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);

    delete sellOrder1;
    delete sellOrder2;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, LimitSellOrderSweepsMultipleOrdersSameLevel) {
    OrderPtr buyOrder1 = new Order(2, 2, 100, 50, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr buyOrder2 = new Order(3, 3, 100, 10, Side::Buy, OrderType::Limit, 1622547802);
    OrderPtr sellOrder = new Order(1, 1, 100, 55, Side::Sell, OrderType::Limit, 1622547803);

    engine->matchOrder(buyOrder1);
    engine->matchOrder(buyOrder2);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder1->getQty(), 0u);
    EXPECT_EQ(buyOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder1;
    delete buyOrder2;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, LimitBuyOrderDoesNotMatch) {
    OrderPtr sellOrder1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = new Order(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder1 = new Order(3, 3, 99, 10, Side::Buy, OrderType::Limit, 1622547802);
    OrderPtr buyOrder2 = new Order(4, 4, 98, 10, Side::Buy, OrderType::Limit, 1622547803);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder1);
    engine->matchOrder(buyOrder2);
    
    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder1->getQty(), 10u);
    EXPECT_EQ(buyOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder2->getQty(), 10u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder1->getQty(), 10u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder2->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestBid(), 99u);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);
    
    delete sellOrder1;
    delete sellOrder2;
    delete buyOrder1;
    delete buyOrder2;
}

TEST_F(MatchingEngineTest, LimitSellOrderDoesNotMatch) {
    OrderPtr buyOrder1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = new Order(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder1 = new Order(3, 3, 101, 10, Side::Sell, OrderType::Limit, 1622547802);
    OrderPtr sellOrder2 = new Order(4, 4, 102, 10, Side::Sell, OrderType::Limit, 1622547803);

    engine->matchOrder(buyOrder1);
    engine->matchOrder(buyOrder2);
    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    
    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder1->getQty(), 10u);
    EXPECT_EQ(buyOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder2->getQty(), 10u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder1->getQty(), 10u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder2->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), 101u);
    
    delete buyOrder1;
    delete buyOrder2;
    delete sellOrder1;
    delete sellOrder2;
}

TEST_F(MatchingEngineTest, MatchMarketBuyOnEmptyBook) {
    OrderPtr buyOrder = new Order(1, 1, 0, 10, Side::Buy, OrderType::Market, 1622547800);

    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder;
}

TEST_F(MatchingEngineTest, MatchMarketSellOnEmptyBook) {
    OrderPtr sellOrder = new Order(1, 1, 0, 10, Side::Sell, OrderType::Market, 1622547800);

    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);

    delete sellOrder;
}

TEST_F(MatchingEngineTest, MarketBuyOrderExactMatch) {
    OrderPtr sellOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = new Order(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete sellOrder;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, MarketSellOrderExactMatch) {
    OrderPtr buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = new Order(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, MarketBuyOrderMatchesBestAsk) {
    OrderPtr sellOrder1 = new Order(1, 1, 103, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = new Order(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = new Order(3, 3, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder1->getQty(), 10u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder2->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 103u);

    delete sellOrder1;
    delete sellOrder2;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, MarketSellOrderMatchesBestAsk) {
    OrderPtr buyOrder1 = new Order(1, 1, 97, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = new Order(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = new Order(3, 3, 0, 10, Side::Sell, OrderType::Market, 1622547802);

    engine->matchOrder(buyOrder1);
    engine->matchOrder(buyOrder2);
    engine->matchOrder(sellOrder);

    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder1->getQty(), 10u);
    EXPECT_EQ(buyOrder2->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder2->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 97u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder1;
    delete buyOrder2;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, RestingBuyOrderPartialFill_IncomingMarketSell) {
    OrderPtr buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = new Order(2, 2, 0, 5, Side::Sell, OrderType::Market, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);

    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, RestingSellOrderPartialFill_IncomingMarketBuy) {
    OrderPtr sellOrder = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = new Order(2, 2, 0, 5, Side::Buy, OrderType::Market, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);

    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);

    delete sellOrder;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, IncomingMarketBuyOrderPartialFill) {
    OrderPtr sellOrder = new Order(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = new Order(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);

    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 5u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete sellOrder;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, IncomingMarketSellOrderPartialFill) {
    OrderPtr buyOrder = new Order(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = new Order(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);

    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 5u);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, MarketBuyOrderSweepsMultipleLevels) {
    OrderPtr sellOrder1 = new Order(1, 1, 100, 50, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = new Order(2, 2, 102, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = new Order(3, 3, 0, 55, Side::Buy, OrderType::Market, 1622547802);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder1->getQty(), 0u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 102u);

    delete sellOrder1;
    delete sellOrder2;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, MarketSellOrderSweepsMultipleLevels) {
    OrderPtr buyOrder1 = new Order(1, 1, 100, 50, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = new Order(2, 2, 98, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = new Order(3, 3, 0, 55, Side::Sell, OrderType::Market, 1622547802);

    engine->matchOrder(buyOrder1);
    engine->matchOrder(buyOrder2);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder1->getQty(), 0u);
    EXPECT_EQ(buyOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 98u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder1;
    delete buyOrder2;
    delete sellOrder;
}

TEST_F(MatchingEngineTest, MarketBuyOrderSweepsMultipleOrdersSameLevel) {
    OrderPtr sellOrder1 = new Order(1, 1, 100, 50, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = new Order(3, 3, 0, 55, Side::Buy, OrderType::Market, 1622547802);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);

    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder1->getQty(), 0u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 0u);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);

    delete sellOrder1;
    delete sellOrder2;
    delete buyOrder;
}

TEST_F(MatchingEngineTest, MarketSellOrderSweepsMultipleOrdersSameLevel) {
    OrderPtr buyOrder1 = new Order(1, 1, 100, 50, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = new Order(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = new Order(3, 3, 0, 55, Side::Sell, OrderType::Market, 1622547802);

    engine->matchOrder(buyOrder1);
    engine->matchOrder(buyOrder2);
    engine->matchOrder(sellOrder);

    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder1->getQty(), 0u);
    EXPECT_EQ(buyOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), 0u);

    delete buyOrder1;
    delete buyOrder2;
    delete sellOrder;
}