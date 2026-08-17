#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"

class MatchingEngineTIFTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    MatchingEngine engine{&orderBook, &stpPolicy};
};

// =====================================================================
// IOC — Limit Buy
// =====================================================================

TEST_F(MatchingEngineTIFTest, IOC_LimitBuy_PartialFill_DiscardsRemainder) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801, 0, TimeInForce::IOC);

    engine.matchOrder(sellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 5);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_LimitBuy_FullFill_Executes) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801, 0, TimeInForce::IOC);

    engine.matchOrder(sellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_LimitBuy_NoLiquidity_CancelledEntirely) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800, 0, TimeInForce::IOC);

    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_LimitBuy_SweepsMultipleLevels_DiscardsRemainder) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 5, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 101, 20, Side::Buy, OrderType::Limit, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_LimitBuy_SweepsMultipleOrdersSameLevel_DiscardsRemainder) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 100, 20, Side::Buy, OrderType::Limit, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

// =====================================================================
// IOC — Limit Sell
// =====================================================================

TEST_F(MatchingEngineTIFTest, IOC_LimitSell_PartialFill_DiscardsRemainder) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801, 0, TimeInForce::IOC);

    engine.matchOrder(buyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 5);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_LimitSell_FullFill_Executes) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801, 0, TimeInForce::IOC);

    engine.matchOrder(buyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_LimitSell_NoLiquidity_CancelledEntirely) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800, 0, TimeInForce::IOC);

    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_LimitSell_SweepsMultipleLevels_DiscardsRemainder) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 5, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 99, 20, Side::Sell, OrderType::Limit, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_LimitSell_SweepsMultipleOrdersSameLevel_DiscardsRemainder) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 100, 20, Side::Sell, OrderType::Limit, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

// =====================================================================
// IOC — Market Buy
// =====================================================================

TEST_F(MatchingEngineTIFTest, IOC_MarketBuy_PartialFill_Discards) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801, 0, TimeInForce::IOC);

    engine.matchOrder(sellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 5);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_MarketBuy_FullFill_Executes) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801, 0, TimeInForce::IOC);

    engine.matchOrder(sellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_MarketBuy_NoLiquidity_Cancelled) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 0, 10, Side::Buy, OrderType::Market, 1622547800, 0, TimeInForce::IOC);

    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_MarketBuy_SweepsMultipleLevels_DiscardsRemainder) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 5, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 0, 20, Side::Buy, OrderType::Market, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_MarketBuy_SweepsMultipleOrdersSameLevel_DiscardsRemainder) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 0, 20, Side::Buy, OrderType::Market, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

// =====================================================================
// IOC — Market Sell
// =====================================================================

TEST_F(MatchingEngineTIFTest, IOC_MarketSell_PartialFill_Discards) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801, 0, TimeInForce::IOC);

    engine.matchOrder(buyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 5);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_MarketSell_FullFill_Executes) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801, 0, TimeInForce::IOC);

    engine.matchOrder(buyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_MarketSell_NoLiquidity_Cancelled) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 0, 10, Side::Sell, OrderType::Market, 1622547800, 0, TimeInForce::IOC);

    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_MarketSell_SweepsMultipleLevels_DiscardsRemainder) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 5, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 0, 20, Side::Sell, OrderType::Market, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, IOC_MarketSell_SweepsMultipleOrdersSameLevel_DiscardsRemainder) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 0, 20, Side::Sell, OrderType::Market, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

// =====================================================================
// FOK — Limit Buy
// =====================================================================

TEST_F(MatchingEngineTIFTest, FOK_LimitBuy_SingleRestingOrder_FullyFillable_Executes) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 15, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitBuy_InsufficientLiquidity_NoFills) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547801, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitBuy_NoLiquidity_CancelledEntirely) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800, 0, TimeInForce::FOK);

    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitBuy_FullyFillable_MultipleLevels_Executes) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 6, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 8, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 101, 10, Side::Buy, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitBuy_FullyFillable_MultipleOrdersSameLevel_Executes) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 6, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 100, 8, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 100, 10, Side::Buy, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitBuy_ExactBoundary_QuantityEqualsAvailable_Executes) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 4, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 6, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 101, 10, Side::Buy, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitBuy_QuantityOneShortOfAvailable_Cancelled) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 4, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 6, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 101, 11, Side::Buy, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 11);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

// =====================================================================
// FOK — Limit Sell
// =====================================================================

TEST_F(MatchingEngineTIFTest, FOK_LimitSell_SingleRestingOrder_FullyFillable_Executes) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 15, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitSell_InsufficientLiquidity_NoFills) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547801, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitSell_NoLiquidity_CancelledEntirely) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800, 0, TimeInForce::FOK);

    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitSell_FullyFillable_MultipleLevels_Executes) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 6, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 8, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 99, 10, Side::Sell, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitSell_FullyFillable_MultipleOrdersSameLevel_Executes) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 6, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 100, 8, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 100, 10, Side::Sell, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitSell_ExactBoundary_QuantityEqualsAvailable_Executes) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 4, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 6, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 99, 10, Side::Sell, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_LimitSell_QuantityOneShortOfAvailable_Cancelled) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 4, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 6, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 99, 11, Side::Sell, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 11);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

// =====================================================================
// FOK — Market Buy
// =====================================================================

TEST_F(MatchingEngineTIFTest, FOK_MarketBuy_SingleRestingOrder_FullyFillable_Executes) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 15, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketBuy_InsufficientLiquidity_CancelledEntirely) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Buy, OrderType::Market, 1622547801, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketBuy_NoLiquidity_CancelledEntirely) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 0, 10, Side::Buy, OrderType::Market, 1622547800, 0, TimeInForce::FOK);

    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketBuy_FullyFillable_MultipleLevels_Executes) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 6, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 8, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 0, 10, Side::Buy, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketBuy_FullyFillable_MultipleOrdersSameLevel_Executes) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 6, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 100, 8, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 0, 10, Side::Buy, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketBuy_ExactBoundary_QuantityEqualsAvailable_Executes) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 4, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 6, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 0, 10, Side::Buy, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketBuy_QuantityOneShortOfAvailable_Cancelled) {
    OrderPtr sellOrder1 = std::make_shared<Order>(1, 1, 100, 4, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr sellOrder2 = std::make_shared<Order>(2, 2, 101, 6, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 0, 11, Side::Buy, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(sellOrder1);
    engine.matchOrder(sellOrder2);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 11);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

// =====================================================================
// FOK — Market Sell
// =====================================================================

TEST_F(MatchingEngineTIFTest, FOK_MarketSell_SingleRestingOrder_FullyFillable_Executes) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 15, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketSell_InsufficientLiquidity_CancelledEntirely) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1622547801, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketSell_NoLiquidity_CancelledEntirely) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 0, 10, Side::Sell, OrderType::Market, 1622547800, 0, TimeInForce::FOK);

    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketSell_FullyFillable_MultipleLevels_Executes) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 6, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 8, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 0, 10, Side::Sell, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketSell_FullyFillable_MultipleOrdersSameLevel_Executes) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 6, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 100, 8, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 0, 10, Side::Sell, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketSell_ExactBoundary_QuantityEqualsAvailable_Executes) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 4, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 6, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 0, 10, Side::Sell, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, FOK_MarketSell_QuantityOneShortOfAvailable_Cancelled) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 4, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 99, 6, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 0, 11, Side::Sell, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(buyOrder1);
    engine.matchOrder(buyOrder2);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 11);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

// =====================================================================
// Self-Trade x TIF — IOC
// =====================================================================

TEST_F(MatchingEngineTIFTest, SelfTrade_IOC_LimitBuy_CancelsSelfTradeThenDiscardsRemainder) {
    OrderPtr otherSellOrder = std::make_shared<Order>(1, 2, 100, 3, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr selfSellOrder = std::make_shared<Order>(2, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(otherSellOrder);
    engine.matchOrder(selfSellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 7);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_IOC_LimitSell_CancelsSelfTradeThenDiscardsRemainder) {
    OrderPtr otherBuyOrder = std::make_shared<Order>(1, 2, 100, 3, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr selfBuyOrder = std::make_shared<Order>(2, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(otherBuyOrder);
    engine.matchOrder(selfBuyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 7);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_IOC_MarketBuy_CancelsSelfTradeThenDiscardsRemainder) {
    OrderPtr otherSellOrder = std::make_shared<Order>(1, 2, 100, 3, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr selfSellOrder = std::make_shared<Order>(2, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(otherSellOrder);
    engine.matchOrder(selfSellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(buyOrder->getQty(), 7);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_IOC_MarketSell_CancelsSelfTradeThenDiscardsRemainder) {
    OrderPtr otherBuyOrder = std::make_shared<Order>(1, 2, 100, 3, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr selfBuyOrder = std::make_shared<Order>(2, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 1, 0, 10, Side::Sell, OrderType::Market, 1622547802, 0, TimeInForce::IOC);

    engine.matchOrder(otherBuyOrder);
    engine.matchOrder(selfBuyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(sellOrder->getQty(), 7);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

// =====================================================================
// Self-Trade x TIF — FOK
// =====================================================================

TEST_F(MatchingEngineTIFTest, SelfTrade_FOK_LimitBuy_AbortsAsUnfillable) {
    OrderPtr selfSellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr otherSellOrder = std::make_shared<Order>(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 1, 101, 5, Side::Buy, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(selfSellOrder);
    engine.matchOrder(otherSellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 5);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_FOK_LimitBuy_EarlyExit_FillsWithoutTouchingSelfLevel) {
    OrderPtr otherSellOrder = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr selfSellOrder = std::make_shared<Order>(2, 1, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 1, 101, 10, Side::Buy, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(otherSellOrder);
    engine.matchOrder(selfSellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_FOK_LimitSell_AbortsAsUnfillable) {
    OrderPtr selfBuyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr otherBuyOrder = std::make_shared<Order>(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 1, 99, 5, Side::Sell, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(selfBuyOrder);
    engine.matchOrder(otherBuyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 5);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_FOK_LimitSell_EarlyExit_FillsWithoutTouchingSelfLevel) {
    OrderPtr otherBuyOrder = std::make_shared<Order>(1, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr selfBuyOrder = std::make_shared<Order>(2, 1, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 1, 99, 10, Side::Sell, OrderType::Limit, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(otherBuyOrder);
    engine.matchOrder(selfBuyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_FOK_MarketBuy_AbortsAsUnfillable) {
    OrderPtr selfSellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr otherSellOrder = std::make_shared<Order>(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 1, 0, 5, Side::Buy, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(selfSellOrder);
    engine.matchOrder(otherSellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(buyOrder->getQty(), 5);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_FOK_MarketBuy_EarlyExit_FillsWithoutTouchingSelfLevel) {
    OrderPtr otherSellOrder = std::make_shared<Order>(1, 2, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr selfSellOrder = std::make_shared<Order>(2, 1, 101, 10, Side::Sell, OrderType::Limit, 1622547801);
    OrderPtr buyOrder = std::make_shared<Order>(3, 1, 0, 10, Side::Buy, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(otherSellOrder);
    engine.matchOrder(selfSellOrder);
    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(buyOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_FOK_MarketSell_AbortsAsUnfillable) {
    OrderPtr selfBuyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr otherBuyOrder = std::make_shared<Order>(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 1, 0, 5, Side::Sell, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(selfBuyOrder);
    engine.matchOrder(otherBuyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::FOKInsufficientLiquidity);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(sellOrder->getQty(), 5);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, SelfTrade_FOK_MarketSell_EarlyExit_FillsWithoutTouchingSelfLevel) {
    OrderPtr otherBuyOrder = std::make_shared<Order>(1, 2, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr selfBuyOrder = std::make_shared<Order>(2, 1, 99, 10, Side::Buy, OrderType::Limit, 1622547801);
    OrderPtr sellOrder = std::make_shared<Order>(3, 1, 0, 10, Side::Sell, OrderType::Market, 1622547802, 0, TimeInForce::FOK);

    engine.matchOrder(otherBuyOrder);
    engine.matchOrder(selfBuyOrder);
    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(sellOrder->getQty(), 0);
    EXPECT_FALSE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}

// =====================================================================
// GTC (default) — remains unaffected by TIF-specific handling
// =====================================================================

TEST_F(MatchingEngineTIFTest, GTC_LimitBuy_RestsNormally) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);

    RejectionReason result = engine.matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder->getQty(), 10);
    EXPECT_TRUE(orderBook.doesOrderExist(buyOrder->getOrderID()));
}

TEST_F(MatchingEngineTIFTest, GTC_LimitSell_RestsNormally) {
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);

    RejectionReason result = engine.matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder->getQty(), 10);
    EXPECT_TRUE(orderBook.doesOrderExist(sellOrder->getOrderID()));
}
