#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"

class MatchingEnginePriceCollarTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    std::optional<MatchingEngine> engine;

    void makeEngine(std::optional<PriceTicks> deviation) {
        engine.emplace(&orderBook, &stpPolicy, nullptr, nullptr, deviation);
    }
};

// =====================================================================
// Max deviation ticks
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, GetMaxDeviationTicks_DefaultsToNullopt) {
    makeEngine(std::nullopt);

    EXPECT_EQ(engine->getMaxDeviationTicks(), std::nullopt);
}

TEST_F(MatchingEnginePriceCollarTest, GetMaxDeviationTicks_ReturnsConstructedValue) {
    makeEngine(5);

    EXPECT_EQ(engine->getMaxDeviationTicks(), 5);
}

// =====================================================================
// Collar disabled
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, Disabled_BuyLimitOrderAtAnyPrice_RestsNormally) {
    makeEngine(std::nullopt);
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePriceCollarTest, Disabled_SellLimitOrderAtAnyPrice_RestsNormally) {
    makeEngine(std::nullopt);
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePriceCollarTest, Disabled_BuyLimitOrderCrossesAtWildPrice_Executes) {
    makeEngine(std::nullopt);
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr buyOrder = std::make_shared<Order>(2, 2, 500, 10, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(sellOrder);
    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePriceCollarTest, Disabled_SellLimitOrderCrossesAtWildPrice_Executes) {
    makeEngine(std::nullopt);
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 1, 10, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(buyOrder);
    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

// =====================================================================
// No reference price available yet
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, NoReference_EmptyBook_BuyAtAnyPriceAccepted) {
    makeEngine(5);
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 500, 10, Side::Buy, OrderType::Limit, 1622547800);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePriceCollarTest, NoReference_EmptyBook_SellAtAnyPriceAccepted) {
    makeEngine(5);
    OrderPtr sellOrder = std::make_shared<Order>(1, 1, 500, 10, Side::Sell, OrderType::Limit, 1622547800);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(1));
}

TEST_F(MatchingEnginePriceCollarTest, NoReference_OneSidedBidOnlyBook_BuyAtAnyPriceAccepted) {
    makeEngine(5);
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr farBuy = std::make_shared<Order>(2, 2, 500, 10, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(restingBuy);
    RejectionReason result = engine->matchOrder(farBuy);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(farBuy->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePriceCollarTest, NoReference_OneSidedAskOnlyBook_SellAtAnyPriceAccepted) {
    makeEngine(5);
    OrderPtr restingSell = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr farSell = std::make_shared<Order>(2, 2, 500, 10, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(restingSell);
    RejectionReason result = engine->matchOrder(farSell);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(farSell->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePriceCollarTest, NoReference_OneSidedAskOnlyBook_MarketableBuyAtWildPriceExecutes) {
    makeEngine(5);
    OrderPtr restingSell = std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800);
    OrderPtr crossingBuy = std::make_shared<Order>(2, 2, 500, 5, Side::Buy, OrderType::Limit, 1622547801);

    engine->matchOrder(restingSell);
    RejectionReason result = engine->matchOrder(crossingBuy);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(crossingBuy->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

TEST_F(MatchingEnginePriceCollarTest, NoReference_OneSidedBidOnlyBook_MarketableSellAtWildPriceExecutes) {
    makeEngine(5);
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr crossingSell = std::make_shared<Order>(2, 2, 1, 5, Side::Sell, OrderType::Limit, 1622547801);

    engine->matchOrder(restingBuy);
    RejectionReason result = engine->matchOrder(crossingSell);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(crossingSell->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(2));
}

// =====================================================================
// Reference = book mid (both sides present, no trade yet)
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, Mid_PriceWithinBand_BuyRestsNormally) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 103, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_PriceWithinBand_SellRestsNormally) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 107, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_PriceAboveBand_BuyRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 116, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_PriceAboveBand_SellRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 116, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_PriceBelowBand_BuyRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 94, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_PriceBelowBand_SellRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 94, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_ExactLowerBoundary_BuyAccepted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 100, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_ExactLowerBoundary_SellAcceptedAndCrossesBid) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 100, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_ExactUpperBoundary_SellAccepted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 110, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_ExactUpperBoundary_BuyAcceptedAndCrossesAsk) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 110, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_JustBelowLowerBoundary_BuyRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 99, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_JustBelowLowerBoundary_SellRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 99, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_JustAboveUpperBoundary_BuyRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 111, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, Mid_JustAboveUpperBoundary_SellRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 111, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

// =====================================================================
// Reference = last traded price, overrides mid once a trade has occurred
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_PriceWithinBand_BuyRestsNormally) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 102, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_PriceWithinBand_SellRestsNormally) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 98, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_PriceAboveBand_BuyRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 112, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_PriceAboveBand_SellRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 112, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_PriceBelowBand_BuyRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 88, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_PriceBelowBand_SellRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 88, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_ExactLowerBoundary_BuyAccepted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 95, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_ExactLowerBoundary_SellAccepted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 95, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_ExactUpperBoundary_BuyAccepted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 105, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_ExactUpperBoundary_SellAccepted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 105, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_JustBelowLowerBoundary_BuyRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 94, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_JustBelowLowerBoundary_SellRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 94, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_JustAboveUpperBoundary_BuyRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 106, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_JustAboveUpperBoundary_SellRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 106, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_ExactLowerBoundary_SellAcceptedAndCrossesBid) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr restingBid = std::make_shared<Order>(3, 3, 95, 5, Side::Buy, OrderType::Limit, 1622547802);
    engine->matchOrder(restingBid);
    OrderPtr sellOrder = std::make_shared<Order>(4, 4, 95, 5, Side::Sell, OrderType::Limit, 1622547803);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(4));
}

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_ExactUpperBoundary_BuyAcceptedAndCrossesAsk) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr restingAsk = std::make_shared<Order>(3, 3, 105, 5, Side::Sell, OrderType::Limit, 1622547802);
    engine->matchOrder(restingAsk);
    OrderPtr buyOrder = std::make_shared<Order>(4, 4, 105, 5, Side::Buy, OrderType::Limit, 1622547803);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(4));
}

// =====================================================================
// Reference price moves as new trades occur
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, LastTradedPrice_ReferenceMoves_PriceValidUnderOldBandNowRejected) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    ASSERT_EQ(engine->getLastTradedPrice(), 100);

    engine->matchOrder(std::make_shared<Order>(3, 3, 95, 5, Side::Sell, OrderType::Limit, 1622547802));
    engine->matchOrder(std::make_shared<Order>(4, 4, 95, 5, Side::Buy, OrderType::Limit, 1622547803));
    ASSERT_EQ(engine->getLastTradedPrice(), 95);

    OrderPtr buyOrder = std::make_shared<Order>(5, 5, 102, 5, Side::Buy, OrderType::Limit, 1622547804);
    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(5));
}

// =====================================================================
// Zero-width band, reference = book mid 
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_Mid_PriceEqualsReference_BuyAccepted) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 105, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_Mid_PriceEqualsReference_SellAccepted) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 105, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_Mid_PriceAboveReference_BuyRejected) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 106, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_Mid_PriceAboveReference_SellRejected) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 106, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_Mid_PriceBelowReference_BuyRejected) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 104, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_Mid_PriceBelowReference_SellRejected) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 104, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

// =====================================================================
// Zero-width band, reference = last traded price 
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_LastTradedPrice_PriceEqualsReference_BuyAccepted) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 100, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_LastTradedPrice_PriceEqualsReference_SellAccepted) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 100, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_LastTradedPrice_PriceAboveReference_BuyRejected) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 101, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_LastTradedPrice_PriceAboveReference_SellRejected) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 101, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_LastTradedPrice_PriceBelowReference_BuyRejected) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr buyOrder = std::make_shared<Order>(3, 3, 99, 5, Side::Buy, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(buyOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, ZeroDeviation_LastTradedPrice_PriceBelowReference_SellRejected) {
    makeEngine(0);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr sellOrder = std::make_shared<Order>(3, 3, 99, 5, Side::Sell, OrderType::Limit, 1622547802);

    RejectionReason result = engine->matchOrder(sellOrder);

    EXPECT_EQ(result, RejectionReason::PriceCollarViolation);
    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

// =====================================================================
// Market orders bypass the collar
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, MarketOrder_NoLiquidity_BuyCancelled) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));

    OrderPtr marketBuy = std::make_shared<Order>(3, 3, 0, 5, Side::Buy, OrderType::Market, 1622547802);
    RejectionReason result = engine->matchOrder(marketBuy);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, MarketOrder_NoLiquidity_SellCancelled) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));

    OrderPtr marketSell = std::make_shared<Order>(3, 3, 0, 5, Side::Sell, OrderType::Market, 1622547802);
    RejectionReason result = engine->matchOrder(marketSell);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
}

TEST_F(MatchingEnginePriceCollarTest, MarketOrder_PartialLiquidity_BuyPartiallyExecuted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr restingSell = std::make_shared<Order>(3, 3, 100, 5, Side::Sell, OrderType::Limit, 1622547802);
    engine->matchOrder(restingSell);

    OrderPtr marketBuy = std::make_shared<Order>(4, 4, 0, 10, Side::Buy, OrderType::Market, 1622547803);
    RejectionReason result = engine->matchOrder(marketBuy);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_FALSE(orderBook.doesOrderExist(4));
}

TEST_F(MatchingEnginePriceCollarTest, MarketOrder_PartialLiquidity_SellPartiallyExecuted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr restingBuy = std::make_shared<Order>(3, 3, 100, 5, Side::Buy, OrderType::Limit, 1622547802);
    engine->matchOrder(restingBuy);

    OrderPtr marketSell = std::make_shared<Order>(4, 4, 0, 10, Side::Sell, OrderType::Market, 1622547803);
    RejectionReason result = engine->matchOrder(marketSell);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(marketSell->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_FALSE(orderBook.doesOrderExist(4));
}

TEST_F(MatchingEnginePriceCollarTest, MarketOrder_FullLiquidity_BuyFullyExecuted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr restingSell = std::make_shared<Order>(3, 3, 100, 5, Side::Sell, OrderType::Limit, 1622547802);
    engine->matchOrder(restingSell);

    OrderPtr marketBuy = std::make_shared<Order>(4, 4, 0, 5, Side::Buy, OrderType::Market, 1622547803);
    RejectionReason result = engine->matchOrder(marketBuy);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(4));
}

TEST_F(MatchingEnginePriceCollarTest, MarketOrder_FullLiquidity_SellFullyExecuted) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr restingBuy = std::make_shared<Order>(3, 3, 100, 5, Side::Buy, OrderType::Limit, 1622547802);
    engine->matchOrder(restingBuy);

    OrderPtr marketSell = std::make_shared<Order>(4, 4, 0, 5, Side::Sell, OrderType::Market, 1622547803);
    RejectionReason result = engine->matchOrder(marketSell);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Executed);
    EXPECT_FALSE(orderBook.doesOrderExist(4));
}

// =====================================================================
// Cancel orders bypass the collar
// =====================================================================

TEST_F(MatchingEnginePriceCollarTest, CancelOrder_BypassesCollar_WhenNoReferenceExists) {
    makeEngine(5);
    OrderPtr restingBuy = std::make_shared<Order>(1, 1, 500, 5, Side::Buy, OrderType::Limit, 1622547800);
    engine->matchOrder(restingBuy);
    OrderPtr cancelOrder = std::make_shared<Order>(2, 1, 0, 0, Side::None, OrderType::Cancel, 1622547801, 1);

    RejectionReason result = engine->matchOrder(cancelOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_FALSE(orderBook.doesOrderExist(1));
    EXPECT_EQ(orderBook.getOrderCancellationCount(), 1);
}

TEST_F(MatchingEnginePriceCollarTest, CancelOrder_BypassesCollar_WhenReferenceEstablished) {
    makeEngine(5);
    engine->matchOrder(std::make_shared<Order>(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1622547800));
    engine->matchOrder(std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1622547801));
    OrderPtr restingBuy = std::make_shared<Order>(3, 3, 102, 5, Side::Buy, OrderType::Limit, 1622547802);
    engine->matchOrder(restingBuy);
    OrderPtr cancelOrder = std::make_shared<Order>(4, 3, 0, 0, Side::None, OrderType::Cancel, 1622547803, 3);

    RejectionReason result = engine->matchOrder(cancelOrder);

    EXPECT_EQ(result, RejectionReason::None);
    EXPECT_FALSE(orderBook.doesOrderExist(3));
    EXPECT_EQ(orderBook.getOrderCancellationCount(), 1);
}
