#include <gtest/gtest.h>
#include <memory>
#include "models/matching_engine.hpp"

class MatchingEngineExecutionTest : public ::testing::Test {
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

TEST_F(MatchingEngineExecutionTest, RecordExecution_Increments_TradeCount_And_Volume) {
	OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
	OrderPtr order2 = std::make_shared<Order>(2, 2, 101, 5, Side::Sell, OrderType::Limit, 1622547801);
	OrderPtr order3 = std::make_shared<Order>(3, 3, 105, 12, Side::Buy, OrderType::Limit, 1622547802);

	engine->matchOrder(order1);
	engine->matchOrder(order2);

	EXPECT_EQ(orderBook->getTradeExecutionCount(), 0u);
	EXPECT_EQ(orderBook->getTotalVolumeTraded(), 0u);

	engine->matchOrder(order3);

	EXPECT_EQ(orderBook->getTradeExecutionCount(), 2u);
	EXPECT_EQ(orderBook->getTotalVolumeTraded(), 12u);
}

TEST_F(MatchingEngineExecutionTest, RecordExecution_DoesNotIncrement_WhenBidsAndAsksNeverCross) {
	OrderPtr order1 = std::make_shared<Order>(1, 1, 99, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr order2 = std::make_shared<Order>(2, 2, 98, 5, Side::Buy, OrderType::Limit, 1622547801);

	engine->matchOrder(order1);
	engine->matchOrder(order2);

	EXPECT_EQ(orderBook->getTradeExecutionCount(), 0u);
	EXPECT_EQ(orderBook->getTotalVolumeTraded(), 0u);

	OrderPtr order3 = std::make_shared<Order>(3, 3, 101, 12, Side::Sell, OrderType::Limit, 1622547802);
	OrderPtr order4 = std::make_shared<Order>(4, 4, 102, 8, Side::Sell, OrderType::Limit, 1622547803);

    engine->matchOrder(order3);
    engine->matchOrder(order4);

    EXPECT_EQ(orderBook->getTradeExecutionCount(), 0u);
    EXPECT_EQ(orderBook->getTotalVolumeTraded(), 0u);
}

TEST_F(MatchingEngineExecutionTest, RecordExecution_DoesNotIncrement_WhenNoOpposingLiquidityExists) {
	OrderPtr order1 = std::make_shared<Order>(1, 1, 99, 10, Side::Buy, OrderType::Market, 1622547800);
	OrderPtr order2 = std::make_shared<Order>(2, 2, 0, 5, Side::Buy, OrderType::Market, 1622547801);

	engine->matchOrder(order1);
	engine->matchOrder(order2);

	EXPECT_EQ(orderBook->getTradeExecutionCount(), 0u);
	EXPECT_EQ(orderBook->getTotalVolumeTraded(), 0u);

	OrderPtr order3 = std::make_shared<Order>(3, 3, 101, 12, Side::Buy, OrderType::Limit, 1622547802);
	OrderPtr order4 = std::make_shared<Order>(4, 4, 0, 8, Side::Buy, OrderType::Market, 1622547803);

    engine->matchOrder(order3);
    engine->matchOrder(order4);

    EXPECT_EQ(orderBook->getTradeExecutionCount(), 0u);
    EXPECT_EQ(orderBook->getTotalVolumeTraded(), 0u);
}

TEST_F(MatchingEngineExecutionTest, RecordCancellation_Cancels_RestingOrder_OnCancelOrder) {
	OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
	OrderPtr order2 = std::make_shared<Order>(2, 1, 0, 0, Side::None, OrderType::Cancel, 1622547801, 1);

	engine->matchOrder(order1);
	engine->matchOrder(order2);

	EXPECT_EQ(orderBook->getOrderCancellationCount(), 1u);
	EXPECT_FALSE(orderBook->doesOrderExist(1));
}

TEST_F(MatchingEngineExecutionTest, RecordCancellation_For_SelfTradePolicy) {
	OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
	OrderPtr order2 = std::make_shared<Order>(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

	engine->matchOrder(order1);
	engine->matchOrder(order2);

	EXPECT_EQ(orderBook->getOrderCancellationCount(), 0u);
}

TEST_F(MatchingEngineExecutionTest, RecordCancellation_DoesNotIncrement_WhenInvalidOrder) {
    OrderPtr order = std::make_shared<Order>(1, 1, -100, 10, Side::Sell, OrderType::Limit, 1622547800);

    engine->matchOrder(order);

    EXPECT_EQ(order->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(orderBook->getOrderCancellationCount(), 0u);
}

TEST_F(MatchingEngineExecutionTest, RecordCancellation_DoesNotIncrement_WhenOrderDoesNotExist) {
    OrderID nonExistentOrderID = 999;
	OrderPtr cancelOrder = std::make_shared<Order>(1, 1, 0, 0, Side::None, OrderType::Cancel, 1622547800, nonExistentOrderID);

    engine->matchOrder(cancelOrder);

    EXPECT_EQ(cancelOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(orderBook->getOrderCancellationCount(), 0u);
}

TEST_F(MatchingEngineExecutionTest, RecordCancellation_DoesNotIncrement_WhenOrderAlreadyExists) {
	OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);

    engine->matchOrder(order);
    engine->matchOrder(order);

    EXPECT_EQ(orderBook->getOrderCancellationCount(), 0u);
}

TEST_F(MatchingEngineExecutionTest, GetLastTradedPrice_InitiallyNullopt) {
    EXPECT_EQ(engine->getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, GetLastTradedPrice_UnchangedWhenOrderRestsWithoutMatching) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);

    engine->matchOrder(buyOrder);

    EXPECT_EQ(engine->getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, GetLastTradedPrice_SetAfterFirstTrade) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);

    EXPECT_EQ(engine->getLastTradedPrice(), 100);
}

TEST_F(MatchingEngineExecutionTest, GetLastTradedPrice_UpdatesToMostRecentTrade) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder1 = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder2 = std::make_shared<Order>(3, 3, 110, 5, Side::Sell, OrderType::Limit, 1622547802);
    OrderPtr buyOrder2 = std::make_shared<Order>(4, 4, 110, 5, Side::Buy, OrderType::Limit, 1622547803);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(buyOrder1);
    EXPECT_EQ(engine->getLastTradedPrice(), 100);

    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder2);
    EXPECT_EQ(engine->getLastTradedPrice(), 110);
}

TEST_F(MatchingEngineExecutionTest, GetLastTradedPrice_ReflectsMakerPriceNotTakerPrice) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 105, 10, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(sellOrder);
    engine->matchOrder(buyOrder);

    EXPECT_EQ(engine->getLastTradedPrice(), 100);
    EXPECT_NE(engine->getLastTradedPrice(), 105);
}

TEST_F(MatchingEngineExecutionTest, GetLastTradedPrice_UpdatesAcrossMultipleLevelsInOneSweep) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 105, 5, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 105, 10, Side::Buy, OrderType::Limit, 1622547802);

    engine->matchOrder(sellOrder1);
    engine->matchOrder(sellOrder2);
    engine->matchOrder(buyOrder);

    EXPECT_EQ(engine->getLastTradedPrice(), 105);
}

TEST_F(MatchingEngineExecutionTest, GetLastTradedPrice_UnaffectedBySelfTradeCancelWithNoFill) {
    OrderPtr selfSellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr selfBuyOrder = std::make_shared<Order>(2, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(selfSellOrder);
    engine->matchOrder(selfBuyOrder);

    EXPECT_EQ(engine->getLastTradedPrice(), std::nullopt);
}
