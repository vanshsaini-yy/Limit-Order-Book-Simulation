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
		engine = new MatchingEngine(stpPolicy, orderBook);
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

TEST_F(MatchingEngineExecutionTest, MatchingEngineExecutionTest_RecordExecution_DoesNotIncrement_WhenNoTradesOccur_1) {
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

TEST_F(MatchingEngineExecutionTest, MatchingEngineExecutionTest_RecordExecution_DoesNotIncrement_WhenNoTradesOccur_2) {
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
