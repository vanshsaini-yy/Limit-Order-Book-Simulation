#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"

class MatchingEngineMatchTest : public ::testing::Test {
protected:
    LimitOrderBook* orderBook;
    STPPolicy* stpPolicy;
    MatchingEngine* engine;

    void SetUp() override {
        stpPolicy = new CancelBothSTP();
        orderBook = new LimitOrderBook();
        engine = new MatchingEngine(orderBook, stpPolicy);
    }

    void TearDown() override {
        delete engine;
        delete orderBook;
        delete stpPolicy;
    }
};

TEST_F(MatchingEngineMatchTest, MatchLimitBuyToEmptyBook) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);

    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MatchOrder_LimitBuy_On_BookContaining_OnlyLimitBuyOrders) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 101, 5, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(buyOrder1);
    engine->matchOrder(buyOrder2);

    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder1->getQty(), 10u);
    EXPECT_EQ(buyOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 101u);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MatchOrder_LimitSell_On_BookContaining_OnlyLimitSellOrders) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 99, 5, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);

    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder1->getQty(), 10u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 99u);
}

TEST_F(MatchingEngineMatchTest, MatchLimitSellToEmptyBook) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);

    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, LimitBuyOrderExactMatch) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801);
    
    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, LimitSellOrderExactMatch) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, LimitBuyOrderMatchesBestAsk) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 103, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 105, 10, Side::Buy, OrderType::Limit, 1622547802);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder1->getQty(), 10u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder2->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 103u);
}

TEST_F(MatchingEngineMatchTest, LimitSellOrderMatchesBestBid) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 97, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 95, 10, Side::Sell, OrderType::Limit, 1622547802);

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
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, RestingBuyOrderPartialFill_IncomingLimitSell) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder->getQty(), 5u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, RestingSellOrderPartialFill_IncomingLimitBuy) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);
}

TEST_F(MatchingEngineMatchTest, IncomingLimitBuyOrderPartialFill) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder->getQty(), 5u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, IncomingLimitSellOrderPartialFill) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder->getQty(), 5u);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);
}

TEST_F(MatchingEngineMatchTest, LimitBuyOrderSweepsMultipleLevels) {
    OrderPtr sellOrder1 = std::make_shared<Order>(2, 2, 100, 50, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr sellOrder2 = std::make_shared<Order>(3, 3, 102, 10, Side::Sell, OrderType::Limit, 1622547802);
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 103, 55, Side::Buy, OrderType::Limit, 1622547803);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder1->getQty(), 0u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 102u);
}

TEST_F(MatchingEngineMatchTest, LimitSellOrderSweepsMultipleLevels) {
    OrderPtr buyOrder1 = std::make_shared<Order>(2, 2, 100, 50, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr buyOrder2 = std::make_shared<Order>(3, 3, 98, 10, Side::Buy, OrderType::Limit, 1622547802);
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 97, 55, Side::Sell, OrderType::Limit, 1622547803);

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
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, LimitBuyOrderSweepsMultipleOrdersSameLevel) {
    OrderPtr sellOrder1 = std::make_shared<Order>(2, 2, 100, 50, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr sellOrder2 = std::make_shared<Order>(3, 3, 100, 10, Side::Sell, OrderType::Limit, 1622547802);
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 55, Side::Buy, OrderType::Limit, 1622547803);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder1->getQty(), 0u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);
}

TEST_F(MatchingEngineMatchTest, LimitSellOrderSweepsMultipleOrdersSameLevel) {
    OrderPtr buyOrder1 = std::make_shared<Order>(2, 2, 100, 50, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr buyOrder2 = std::make_shared<Order>(3, 3, 100, 10, Side::Buy, OrderType::Limit, 1622547802);
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 55, Side::Sell, OrderType::Limit, 1622547803);

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
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, LimitBuyOrderDoesNotMatch) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder1 = std::make_shared<Order>(3, 3, 99, 10, Side::Buy, OrderType::Limit, 1622547802);
    OrderPtr buyOrder2 = std::make_shared<Order>(4, 4, 98, 10, Side::Buy, OrderType::Limit, 1622547803);

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
}

TEST_F(MatchingEngineMatchTest, LimitSellOrderDoesNotMatch) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder1 = std::make_shared<Order>(3, 3, 101, 10, Side::Sell, OrderType::Limit, 1622547802);
    OrderPtr sellOrder2 = std::make_shared<Order>(4, 4, 102, 10, Side::Sell, OrderType::Limit, 1622547803);

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
}

TEST_F(MatchingEngineMatchTest, MatchMarketBuyOnEmptyBook) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 0, 10, Side::Buy, OrderType::Market, 1622547800);

    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MatchMarketSellOnEmptyBook) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 0, 10, Side::Sell, OrderType::Market, 1622547800);

    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MatchOrder_MarketBuy_On_BookContainingOnlyLimitBuy) {
    OrderPtr limitBuyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr marketBuyOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engine->matchOrder(limitBuyOrder);
    engine->matchOrder(marketBuyOrder);

    EXPECT_EQ(marketBuyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(marketBuyOrder->getQty(), 10u);
    EXPECT_EQ(limitBuyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(limitBuyOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MatchOrder_MarketSell_On_BookContainingOnlyLimitSell) {
    OrderPtr limitSellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr marketSellOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801);

    engine->matchOrder(limitSellOrder);
    engine->matchOrder(marketSellOrder);

    EXPECT_EQ(marketSellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(marketSellOrder->getQty(), 10u);
    EXPECT_EQ(limitSellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(limitSellOrder->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MarketBuyOrderExactMatch) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MarketSellOrderExactMatch) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);
    
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MarketBuyOrderMatchesBestAsk) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 103, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 0, 10, Side::Buy, OrderType::Market, 1622547802);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder1->getQty(), 10u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder2->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 103u);
}

TEST_F(MatchingEngineMatchTest, MarketSellOrderMatchesBestAsk) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 97, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 0, 10, Side::Sell, OrderType::Market, 1622547802);

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
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, RestingBuyOrderPartialFill_IncomingMarketSell) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 0, 5, Side::Sell, OrderType::Market, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);

    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(buyOrder->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), 100u);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, RestingSellOrderPartialFill_IncomingMarketBuy) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 0, 5, Side::Buy, OrderType::Market, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);

    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);
}

TEST_F(MatchingEngineMatchTest, IncomingMarketBuyOrderPartialFill) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);

    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 5u);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, IncomingMarketSellOrderPartialFill) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801);

    engine->matchOrder(buyOrder);
    engine->matchOrder(sellOrder);

    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 5u);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MarketBuyOrderSweepsMultipleLevels) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 50, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 102, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 0, 55, Side::Buy, OrderType::Market, 1622547802);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);
    
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder1->getQty(), 0u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 102u);
}

TEST_F(MatchingEngineMatchTest, MarketSellOrderSweepsMultipleLevels) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 50, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 98, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 0, 55, Side::Sell, OrderType::Market, 1622547802);

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
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MarketBuyOrderSweepsMultipleOrdersSameLevel) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 50, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 0, 55, Side::Buy, OrderType::Market, 1622547802);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);

    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0u);
    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder1->getQty(), 0u);
    EXPECT_EQ(sellOrder2->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(sellOrder2->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), 100u);
}

TEST_F(MatchingEngineMatchTest, MarketSellOrderSweepsMultipleOrdersSameLevel) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 50, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 0, 55, Side::Sell, OrderType::Market, 1622547802);

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
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, MultipleMarketOrders) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 0, 10, Side::Sell, OrderType::Market, 1622547800);
    OrderPtr buyOrder1 = std::make_shared<Order>(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(buyOrder1);

    EXPECT_EQ(sellOrder1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder1->getQty(), 10u);
    EXPECT_EQ(buyOrder1->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder1->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, CancelOrder_Cancels_Pending_RestingOrder) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    engine->matchOrder(order);
    OrderPtr cancelOrder = std::make_shared<Order>(2, 1, 0, 0, Side::None, OrderType::Cancel, 1622547801, order->getOrderID());
    engine->matchOrder(cancelOrder);

    EXPECT_EQ(cancelOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(order->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook->doesOrderExist(order->getOrderID()));
    EXPECT_EQ(order->getQty(), 10u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, CancelOrder_DoesNotCancel_OrderOwnedByAnotherUser) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    engine->matchOrder(order);
    OrderPtr cancelOrder = std::make_shared<Order>(2, 2, 0, 0, Side::None, OrderType::Cancel, 1622547801, order->getOrderID());

    EXPECT_EQ(engine->matchOrder(cancelOrder), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(cancelOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(order->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook->doesOrderExist(order->getOrderID()));
    EXPECT_EQ(orderBook->getBestBid(), 100u);
}

TEST_F(MatchingEngineMatchTest, CancelOrder_Cancels_PartiallyExecuted_RestingOrder) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr order2 = std::make_shared<Order>(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr cancelOrder = std::make_shared<Order>(3, 1, 0, 0, Side::None, OrderType::Cancel, 1622547802, order1->getOrderID());
    
    engine->matchOrder(order1);
    engine->matchOrder(order2);
    engine->matchOrder(cancelOrder);

    EXPECT_EQ(cancelOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(order1->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_FALSE(orderBook->doesOrderExist(order1->getOrderID()));
    EXPECT_EQ(order1->getQty(), 5u);
    EXPECT_EQ(orderBook->getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook->getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, CancelOrder_DoesNotCancel_NonExistentOrder) {
    OrderID nonExistentOrderID = 999;
    OrderPtr cancelOrder = std::make_shared<Order>(1, 1, 0, 0, Side::None, OrderType::Cancel, 1622547800, nonExistentOrderID);
    
    EXPECT_EQ(engine->matchOrder(cancelOrder), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(cancelOrder->getStatus(), OrderStatus::Cancelled);
}