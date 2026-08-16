#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"

class MatchingEnginePostOnlyTest : public ::testing::Test {
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

// =====================================================================
// Post-Only — Limit Buy
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, PostOnly_LimitBuy_WouldCross_Rejected) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801, 0, TimeInForce::GTC, true);

    engine->matchOrder(sellOrder);
    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10u);
    EXPECT_FALSE(orderBook->doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEnginePostOnlyTest, PostOnly_LimitBuy_WouldCross_AboveBestAsk_Rejected) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 101, 10, Side::Buy, OrderType::Limit, 1622547801, 0, TimeInForce::GTC, true);

    engine->matchOrder(sellOrder);
    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10u);
    EXPECT_FALSE(orderBook->doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEnginePostOnlyTest, PostOnly_LimitBuy_WouldNotCross_RestsNormally) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1622547801, 0, TimeInForce::GTC, true);

    engine->matchOrder(sellOrder);
    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder->getQty(), 10u);
    EXPECT_TRUE(orderBook->doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEnginePostOnlyTest, PostOnly_LimitBuy_EmptyBook_RestsNormally) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800, 0, TimeInForce::GTC, true);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder->getQty(), 10u);
    EXPECT_TRUE(orderBook->doesOrderExist(buyOrder->getOrderID()));
}

// =====================================================================
// Post-Only — Limit Sell
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, PostOnly_LimitSell_WouldCross_Rejected) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801, 0, TimeInForce::GTC, true);

    engine->matchOrder(buyOrder);
    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10u);
    EXPECT_FALSE(orderBook->doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEnginePostOnlyTest, PostOnly_LimitSell_WouldCross_BelowBestBid_Rejected) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 99, 10, Side::Sell, OrderType::Limit, 1622547801, 0, TimeInForce::GTC, true);

    engine->matchOrder(buyOrder);
    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PostOnlyWouldCross);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10u);
    EXPECT_FALSE(orderBook->doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEnginePostOnlyTest, PostOnly_LimitSell_WouldNotCross_RestsNormally) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801, 0, TimeInForce::GTC, true);

    engine->matchOrder(buyOrder);
    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder->getQty(), 10u);
    EXPECT_TRUE(orderBook->doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEnginePostOnlyTest, PostOnly_LimitSell_EmptyBook_RestsNormally) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800, 0, TimeInForce::GTC, true);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder->getQty(), 10u);
    EXPECT_TRUE(orderBook->doesOrderExist(sellOrder->getOrderID()));
}

// =====================================================================
// Post-Only — Validation
// =====================================================================

TEST_F(MatchingEnginePostOnlyTest, PostOnly_CombinedWithIOC_Rejected) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800, 0, TimeInForce::IOC, true);

    RejectionReason result = engine->matchOrder(order);

    EXPECT_EQ(result, RejectionReason::InvalidPostOnlyOrder);
    EXPECT_EQ(order->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook->doesOrderExist(order->getOrderID()));
}

TEST_F(MatchingEnginePostOnlyTest, PostOnly_CombinedWithFOK_Rejected) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800, 0, TimeInForce::FOK, true);

    RejectionReason result = engine->matchOrder(order);

    EXPECT_EQ(result, RejectionReason::InvalidPostOnlyOrder);
    EXPECT_EQ(order->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook->doesOrderExist(order->getOrderID()));
}

TEST_F(MatchingEnginePostOnlyTest, PostOnly_OnMarketOrder_Rejected) {
    OrderPtr order = std::make_shared<Order>(1, 1, 0, 10, Side::Buy, OrderType::Market, 1622547800, 0, TimeInForce::GTC, true);

    RejectionReason result = engine->matchOrder(order);

    EXPECT_EQ(result, RejectionReason::InvalidPostOnlyOrder);
    EXPECT_EQ(order->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook->doesOrderExist(order->getOrderID()));
}
