#include <gtest/gtest.h>
#include "engine/matching_engine.hpp"
#include "support/new_order.hpp"

class MatchingEngineMatchTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    MatchingEngine engine{&orderBook, &stpPolicy};
};

// =====================================================================
// Limit orders that rest without matching
// =====================================================================

TEST_F(MatchingEngineMatchTest, Submit_LimitBuyOnEmptyBook_RestsInTheBook) {
    OrderID buyOrderID = 1;
    NewOrderRequest buyRequest = newOrderRequestHelper(buyOrderID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);

    OrderPtr buyOrder = engine.submit(buyRequest).order;

    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(buyOrderID));
}

TEST_F(MatchingEngineMatchTest, Submit_LimitSellOnEmptyBook_RestsInTheBook) {
    OrderID sellOrderID = 1;
    NewOrderRequest sellRequest = newOrderRequestHelper(sellOrderID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);

    OrderPtr sellOrder = engine.submit(sellRequest).order;

    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(sellOrderID));
}

TEST_F(MatchingEngineMatchTest, Submit_BuyBelowBestAsk_RestsWithoutMatching) {
    OrderID restingSellID = 1;
    OrderID buyOrderID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest buyRequest = newOrderRequestHelper(buyOrderID, 2, 99, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    OrderPtr buyOrder = engine.submit(buyRequest).order;

    EXPECT_EQ(buyOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(buyOrder->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(buyOrderID));
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(restingSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_SellAboveBestBid_RestsWithoutMatching) {
    OrderID restingBuyID = 1;
    OrderID sellOrderID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest sellRequest = newOrderRequestHelper(sellOrderID, 2, 101, 10, Side::Sell, OrderType::Limit, 1001);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    OrderPtr sellOrder = engine.submit(sellRequest).order;

    EXPECT_EQ(sellOrder->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(sellOrder->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(sellOrderID));
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(restingBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_BuyAboveBestBidOnBookOfOnlyBuys_BecomesNewBestBid) {
    OrderID firstBuyID = 1;
    OrderID secondBuyID = 2;
    NewOrderRequest firstBuyRequest = newOrderRequestHelper(firstBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuyRequest = newOrderRequestHelper(secondBuyID, 2, 101, 5, Side::Buy, OrderType::Limit, 1001);
    OrderPtr firstBuy = engine.submit(firstBuyRequest).order;

    OrderPtr secondBuy = engine.submit(secondBuyRequest).order;

    EXPECT_EQ(firstBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(firstBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(firstBuyID));
    EXPECT_EQ(secondBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(secondBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondBuyID));
    EXPECT_EQ(orderBook.getBestBid(), std::optional<PriceTicks>{101});
    EXPECT_EQ(orderBook.getBestAsk(), std::nullopt);
}

TEST_F(MatchingEngineMatchTest, Submit_SellBelowBestAskOnBookOfOnlySells_BecomesNewBestAsk) {
    OrderID firstSellID = 1;
    OrderID secondSellID = 2;
    NewOrderRequest firstSellRequest = newOrderRequestHelper(firstSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSellRequest = newOrderRequestHelper(secondSellID, 2, 99, 5, Side::Sell, OrderType::Limit, 1001);
    OrderPtr firstSell = engine.submit(firstSellRequest).order;

    OrderPtr secondSell = engine.submit(secondSellRequest).order;

    EXPECT_EQ(firstSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(firstSell->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(firstSellID));
    EXPECT_EQ(secondSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(secondSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondSellID));
    EXPECT_EQ(orderBook.getBestBid(), std::nullopt);
    EXPECT_EQ(orderBook.getBestAsk(), std::optional<PriceTicks>{99});
}

// =====================================================================
// Limit order exact and partial fills
// =====================================================================

TEST_F(MatchingEngineMatchTest, Submit_BuyEqualToRestingSell_BothFullyExecuted) {
    OrderID restingSellID = 1;
    OrderID takerBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest takerBuyRequest = newOrderRequestHelper(takerBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    OrderPtr takerBuy = engine.submit(takerBuyRequest).order;

    EXPECT_EQ(takerBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerBuyID));
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(restingSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_SellEqualToRestingBuy_BothFullyExecuted) {
    OrderID restingBuyID = 1;
    OrderID takerSellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest takerSellRequest = newOrderRequestHelper(takerSellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    OrderPtr takerSell = engine.submit(takerSellRequest).order;

    EXPECT_EQ(takerSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerSellID));
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(restingBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_BuySmallerThanRestingSell_RestingSellPartiallyExecuted) {
    OrderID restingSellID = 1;
    OrderID takerBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest takerBuyRequest = newOrderRequestHelper(takerBuyID, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    OrderPtr takerBuy = engine.submit(takerBuyRequest).order;

    EXPECT_EQ(takerBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerBuyID));
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(restingSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_SellSmallerThanRestingBuy_RestingBuyPartiallyExecuted) {
    OrderID restingBuyID = 1;
    OrderID takerSellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest takerSellRequest = newOrderRequestHelper(takerSellID, 2, 100, 5, Side::Sell, OrderType::Limit, 1001);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    OrderPtr takerSell = engine.submit(takerSellRequest).order;

    EXPECT_EQ(takerSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerSellID));
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(restingBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_BuyLargerThanRestingSell_IncomingBuyRestsTheRemainder) {
    OrderID restingSellID = 1;
    OrderID takerBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest takerBuyRequest = newOrderRequestHelper(takerBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    OrderPtr takerBuy = engine.submit(takerBuyRequest).order;

    EXPECT_EQ(takerBuy->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(takerBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(takerBuyID));
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(restingSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_SellLargerThanRestingBuy_IncomingSellRestsTheRemainder) {
    OrderID restingBuyID = 1;
    OrderID takerSellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest takerSellRequest = newOrderRequestHelper(takerSellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    OrderPtr takerSell = engine.submit(takerSellRequest).order;

    EXPECT_EQ(takerSell->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(takerSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(takerSellID));
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(restingBuyID));
}

// =====================================================================
// Limit order price and time priority
// =====================================================================

TEST_F(MatchingEngineMatchTest, Submit_BuyAcrossTwoAskLevels_TradesWithTheBestAskFirst) {
    OrderID worseSellID = 1;
    OrderID bestSellID = 2;
    OrderID takerBuyID = 3;
    NewOrderRequest worseSellRequest = newOrderRequestHelper(worseSellID, 1, 103, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest bestSellRequest = newOrderRequestHelper(bestSellID, 2, 101, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest takerBuyRequest = newOrderRequestHelper(takerBuyID, 3, 105, 10, Side::Buy, OrderType::Limit, 1002);
    OrderPtr worseSell = engine.submit(worseSellRequest).order;
    OrderPtr bestSell = engine.submit(bestSellRequest).order;

    OrderPtr takerBuy = engine.submit(takerBuyRequest).order;

    EXPECT_EQ(takerBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerBuyID));
    EXPECT_EQ(bestSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(bestSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(bestSellID));
    EXPECT_EQ(worseSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(worseSell->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(worseSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_SellAcrossTwoBidLevels_TradesWithTheBestBidFirst) {
    OrderID worseBuyID = 1;
    OrderID bestBuyID = 2;
    OrderID takerSellID = 3;
    NewOrderRequest worseBuyRequest = newOrderRequestHelper(worseBuyID, 1, 97, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest bestBuyRequest = newOrderRequestHelper(bestBuyID, 2, 99, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest takerSellRequest = newOrderRequestHelper(takerSellID, 3, 95, 10, Side::Sell, OrderType::Limit, 1002);
    OrderPtr worseBuy = engine.submit(worseBuyRequest).order;
    OrderPtr bestBuy = engine.submit(bestBuyRequest).order;

    OrderPtr takerSell = engine.submit(takerSellRequest).order;

    EXPECT_EQ(takerSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerSellID));
    EXPECT_EQ(bestBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(bestBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(bestBuyID));
    EXPECT_EQ(worseBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(worseBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(worseBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_BuyAgainstTwoSellsAtOnePrice_TradesWithTheEarlierSellFirst) {
    OrderID earlierSellID = 1;
    OrderID laterSellID = 2;
    OrderID takerBuyID = 3;
    NewOrderRequest earlierSellRequest = newOrderRequestHelper(earlierSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest laterSellRequest = newOrderRequestHelper(laterSellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest takerBuyRequest = newOrderRequestHelper(takerBuyID, 3, 100, 10, Side::Buy, OrderType::Limit, 1002);
    OrderPtr earlierSell = engine.submit(earlierSellRequest).order;
    OrderPtr laterSell = engine.submit(laterSellRequest).order;

    OrderPtr takerBuy = engine.submit(takerBuyRequest).order;

    EXPECT_EQ(takerBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerBuyID));
    EXPECT_EQ(earlierSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(earlierSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(earlierSellID));
    EXPECT_EQ(laterSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(laterSell->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(laterSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_SellAgainstTwoBuysAtOnePrice_TradesWithTheEarlierBuyFirst) {
    OrderID earlierBuyID = 1;
    OrderID laterBuyID = 2;
    OrderID takerSellID = 3;
    NewOrderRequest earlierBuyRequest = newOrderRequestHelper(earlierBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest laterBuyRequest = newOrderRequestHelper(laterBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest takerSellRequest = newOrderRequestHelper(takerSellID, 3, 100, 10, Side::Sell, OrderType::Limit, 1002);
    OrderPtr earlierBuy = engine.submit(earlierBuyRequest).order;
    OrderPtr laterBuy = engine.submit(laterBuyRequest).order;

    OrderPtr takerSell = engine.submit(takerSellRequest).order;

    EXPECT_EQ(takerSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerSellID));
    EXPECT_EQ(earlierBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(earlierBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(earlierBuyID));
    EXPECT_EQ(laterBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(laterBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(laterBuyID));
}

// =====================================================================
// Limit order sweeps
// =====================================================================

TEST_F(MatchingEngineMatchTest, Submit_BuySweepsTwoAskLevels_LeavesTheSecondLevelPartiallyExecuted) {
    OrderID firstSellID = 1;
    OrderID secondSellID = 2;
    OrderID takerBuyID = 3;
    NewOrderRequest firstSellRequest = newOrderRequestHelper(firstSellID, 1, 100, 50, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSellRequest = newOrderRequestHelper(secondSellID, 2, 102, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest takerBuyRequest = newOrderRequestHelper(takerBuyID, 3, 103, 55, Side::Buy, OrderType::Limit, 1002);
    OrderPtr firstSell = engine.submit(firstSellRequest).order;
    OrderPtr secondSell = engine.submit(secondSellRequest).order;

    OrderPtr takerBuy = engine.submit(takerBuyRequest).order;

    EXPECT_EQ(takerBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerBuyID));
    EXPECT_EQ(firstSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstSellID));
    EXPECT_EQ(secondSell->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(secondSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_SellSweepsTwoBidLevels_LeavesTheSecondLevelPartiallyExecuted) {
    OrderID firstBuyID = 1;
    OrderID secondBuyID = 2;
    OrderID takerSellID = 3;
    NewOrderRequest firstBuyRequest = newOrderRequestHelper(firstBuyID, 1, 100, 50, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuyRequest = newOrderRequestHelper(secondBuyID, 2, 98, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest takerSellRequest = newOrderRequestHelper(takerSellID, 3, 97, 55, Side::Sell, OrderType::Limit, 1002);
    OrderPtr firstBuy = engine.submit(firstBuyRequest).order;
    OrderPtr secondBuy = engine.submit(secondBuyRequest).order;

    OrderPtr takerSell = engine.submit(takerSellRequest).order;

    EXPECT_EQ(takerSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerSellID));
    EXPECT_EQ(firstBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstBuyID));
    EXPECT_EQ(secondBuy->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(secondBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_BuySweepsTwoSellsAtOnePrice_LeavesTheSecondSellPartiallyExecuted) {
    OrderID firstSellID = 1;
    OrderID secondSellID = 2;
    OrderID takerBuyID = 3;
    NewOrderRequest firstSellRequest = newOrderRequestHelper(firstSellID, 1, 100, 50, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSellRequest = newOrderRequestHelper(secondSellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest takerBuyRequest = newOrderRequestHelper(takerBuyID, 3, 100, 55, Side::Buy, OrderType::Limit, 1002);
    OrderPtr firstSell = engine.submit(firstSellRequest).order;
    OrderPtr secondSell = engine.submit(secondSellRequest).order;

    OrderPtr takerBuy = engine.submit(takerBuyRequest).order;

    EXPECT_EQ(takerBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerBuyID));
    EXPECT_EQ(firstSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstSellID));
    EXPECT_EQ(secondSell->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(secondSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_SellSweepsTwoBuysAtOnePrice_LeavesTheSecondBuyPartiallyExecuted) {
    OrderID firstBuyID = 1;
    OrderID secondBuyID = 2;
    OrderID takerSellID = 3;
    NewOrderRequest firstBuyRequest = newOrderRequestHelper(firstBuyID, 1, 100, 50, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuyRequest = newOrderRequestHelper(secondBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest takerSellRequest = newOrderRequestHelper(takerSellID, 3, 100, 55, Side::Sell, OrderType::Limit, 1002);
    OrderPtr firstBuy = engine.submit(firstBuyRequest).order;
    OrderPtr secondBuy = engine.submit(secondBuyRequest).order;

    OrderPtr takerSell = engine.submit(takerSellRequest).order;

    EXPECT_EQ(takerSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(takerSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(takerSellID));
    EXPECT_EQ(firstBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstBuyID));
    EXPECT_EQ(secondBuy->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(secondBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_BuyLargerThanAllAsks_RemainderRestsAtTheBuyLimitPrice) {
    OrderID firstSellID = 1;
    OrderID secondSellID = 2;
    OrderID takerBuyID = 3;
    NewOrderRequest firstSellRequest = newOrderRequestHelper(firstSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSellRequest = newOrderRequestHelper(secondSellID, 2, 101, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest takerBuyRequest = newOrderRequestHelper(takerBuyID, 3, 105, 15, Side::Buy, OrderType::Limit, 1002);
    OrderPtr firstSell = engine.submit(firstSellRequest).order;
    OrderPtr secondSell = engine.submit(secondSellRequest).order;

    OrderPtr takerBuy = engine.submit(takerBuyRequest).order;

    EXPECT_EQ(takerBuy->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(takerBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(takerBuyID));
    EXPECT_EQ(firstSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstSellID));
    EXPECT_EQ(secondSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(secondSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(secondSellID));
    EXPECT_EQ(orderBook.getBestBid(), std::optional<PriceTicks>{105});
}

TEST_F(MatchingEngineMatchTest, Submit_SellLargerThanAllBids_RemainderRestsAtTheSellLimitPrice) {
    OrderID firstBuyID = 1;
    OrderID secondBuyID = 2;
    OrderID takerSellID = 3;
    NewOrderRequest firstBuyRequest = newOrderRequestHelper(firstBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuyRequest = newOrderRequestHelper(secondBuyID, 2, 99, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest takerSellRequest = newOrderRequestHelper(takerSellID, 3, 95, 15, Side::Sell, OrderType::Limit, 1002);
    OrderPtr firstBuy = engine.submit(firstBuyRequest).order;
    OrderPtr secondBuy = engine.submit(secondBuyRequest).order;

    OrderPtr takerSell = engine.submit(takerSellRequest).order;

    EXPECT_EQ(takerSell->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(takerSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(takerSellID));
    EXPECT_EQ(firstBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstBuyID));
    EXPECT_EQ(secondBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(secondBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(secondBuyID));
    EXPECT_EQ(orderBook.getBestAsk(), std::optional<PriceTicks>{95});
}

TEST_F(MatchingEngineMatchTest, Submit_BuySweepStopsAtItsLimitPrice_AsksAboveTheLimitStayUntouched) {
    OrderID firstSellID = 1;
    OrderID secondSellID = 2;
    OrderID thirdSellID = 3;
    OrderID takerBuyID = 4;
    NewOrderRequest firstSellRequest = newOrderRequestHelper(firstSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSellRequest = newOrderRequestHelper(secondSellID, 2, 101, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest thirdSellRequest = newOrderRequestHelper(thirdSellID, 3, 103, 5, Side::Sell, OrderType::Limit, 1002);
    NewOrderRequest takerBuyRequest = newOrderRequestHelper(takerBuyID, 4, 102, 15, Side::Buy, OrderType::Limit, 1003);
    OrderPtr firstSell = engine.submit(firstSellRequest).order;
    OrderPtr secondSell = engine.submit(secondSellRequest).order;
    OrderPtr thirdSell = engine.submit(thirdSellRequest).order;

    OrderPtr takerBuy = engine.submit(takerBuyRequest).order;

    EXPECT_EQ(takerBuy->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(takerBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(takerBuyID));
    EXPECT_EQ(firstSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstSellID));
    EXPECT_EQ(secondSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(secondSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(secondSellID));
    EXPECT_EQ(thirdSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(thirdSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(thirdSellID));
    EXPECT_EQ(orderBook.getBestBid(), std::optional<PriceTicks>{102});
    EXPECT_EQ(orderBook.getBestAsk(), std::optional<PriceTicks>{103});
}

TEST_F(MatchingEngineMatchTest, Submit_SellSweepStopsAtItsLimitPrice_BidsBelowTheLimitStayUntouched) {
    OrderID firstBuyID = 1;
    OrderID secondBuyID = 2;
    OrderID thirdBuyID = 3;
    OrderID takerSellID = 4;
    NewOrderRequest firstBuyRequest = newOrderRequestHelper(firstBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuyRequest = newOrderRequestHelper(secondBuyID, 2, 99, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest thirdBuyRequest = newOrderRequestHelper(thirdBuyID, 3, 97, 5, Side::Buy, OrderType::Limit, 1002);
    NewOrderRequest takerSellRequest = newOrderRequestHelper(takerSellID, 4, 98, 15, Side::Sell, OrderType::Limit, 1003);
    OrderPtr firstBuy = engine.submit(firstBuyRequest).order;
    OrderPtr secondBuy = engine.submit(secondBuyRequest).order;
    OrderPtr thirdBuy = engine.submit(thirdBuyRequest).order;

    OrderPtr takerSell = engine.submit(takerSellRequest).order;

    EXPECT_EQ(takerSell->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(takerSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(takerSellID));
    EXPECT_EQ(firstBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstBuyID));
    EXPECT_EQ(secondBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(secondBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(secondBuyID));
    EXPECT_EQ(thirdBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(thirdBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(thirdBuyID));
    EXPECT_EQ(orderBook.getBestBid(), std::optional<PriceTicks>{97});
    EXPECT_EQ(orderBook.getBestAsk(), std::optional<PriceTicks>{98});
}

// =====================================================================
// Market orders with nothing to trade against
// =====================================================================

TEST_F(MatchingEngineMatchTest, Submit_MarketBuyOnEmptyBook_IsCancelledAndDoesNotRest) {
    OrderID marketBuyID = 1;
    NewOrderRequest marketBuyRequest = newOrderRequestHelper(marketBuyID, 1, 0, 10, Side::Buy, OrderType::Market, 1000);

    OrderPtr marketBuy = engine.submit(marketBuyRequest).order;

    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(marketBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(marketBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketSellOnEmptyBook_IsCancelledAndDoesNotRest) {
    OrderID marketSellID = 1;
    NewOrderRequest marketSellRequest = newOrderRequestHelper(marketSellID, 1, 0, 10, Side::Sell, OrderType::Market, 1000);

    OrderPtr marketSell = engine.submit(marketSellRequest).order;

    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(marketSell->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(marketSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketBuyOnBookOfOnlyBuys_IsCancelledAndLeavesRestingBuyUntouched) {
    OrderID restingBuyID = 1;
    OrderID marketBuyID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest marketBuyRequest = newOrderRequestHelper(marketBuyID, 2, 0, 10, Side::Buy, OrderType::Market, 1001);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    OrderPtr marketBuy = engine.submit(marketBuyRequest).order;

    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(marketBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(marketBuyID));
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(restingBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketSellOnBookOfOnlySells_IsCancelledAndLeavesRestingSellUntouched) {
    OrderID restingSellID = 1;
    OrderID marketSellID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest marketSellRequest = newOrderRequestHelper(marketSellID, 2, 0, 10, Side::Sell, OrderType::Market, 1001);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    OrderPtr marketSell = engine.submit(marketSellRequest).order;

    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Cancelled);
    EXPECT_EQ(marketSell->getQty(), static_cast<Quantity>(10));
    EXPECT_FALSE(orderBook.doesOrderExist(marketSellID));
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(restingSellID));
}

// =====================================================================
// Market order exact and partial fills
// =====================================================================

TEST_F(MatchingEngineMatchTest, Submit_MarketBuyEqualToRestingSell_BothFullyExecuted) {
    OrderID restingSellID = 1;
    OrderID marketBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest marketBuyRequest = newOrderRequestHelper(marketBuyID, 2, 0, 10, Side::Buy, OrderType::Market, 1001);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    OrderPtr marketBuy = engine.submit(marketBuyRequest).order;

    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketBuyID));
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(restingSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketSellEqualToRestingBuy_BothFullyExecuted) {
    OrderID restingBuyID = 1;
    OrderID marketSellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest marketSellRequest = newOrderRequestHelper(marketSellID, 2, 0, 10, Side::Sell, OrderType::Market, 1001);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    OrderPtr marketSell = engine.submit(marketSellRequest).order;

    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketSellID));
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(restingBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketBuySmallerThanRestingSell_RestingSellPartiallyExecuted) {
    OrderID restingSellID = 1;
    OrderID marketBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest marketBuyRequest = newOrderRequestHelper(marketBuyID, 2, 0, 5, Side::Buy, OrderType::Market, 1001);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    OrderPtr marketBuy = engine.submit(marketBuyRequest).order;

    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketBuyID));
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(restingSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketSellSmallerThanRestingBuy_RestingBuyPartiallyExecuted) {
    OrderID restingBuyID = 1;
    OrderID marketSellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest marketSellRequest = newOrderRequestHelper(marketSellID, 2, 0, 5, Side::Sell, OrderType::Market, 1001);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    OrderPtr marketSell = engine.submit(marketSellRequest).order;

    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketSellID));
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(restingBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketBuyLargerThanRestingSell_MarketBuyCancelledAfterPartialExecution) {
    OrderID restingSellID = 1;
    OrderID marketBuyID = 2;
    NewOrderRequest restingSellRequest = newOrderRequestHelper(restingSellID, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest marketBuyRequest = newOrderRequestHelper(marketBuyID, 2, 0, 10, Side::Buy, OrderType::Market, 1001);
    OrderPtr restingSell = engine.submit(restingSellRequest).order;

    OrderPtr marketBuy = engine.submit(marketBuyRequest).order;

    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(marketBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_FALSE(orderBook.doesOrderExist(marketBuyID));
    EXPECT_EQ(restingSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(restingSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketSellLargerThanRestingBuy_MarketSellCancelledAfterPartialExecution) {
    OrderID restingBuyID = 1;
    OrderID marketSellID = 2;
    NewOrderRequest restingBuyRequest = newOrderRequestHelper(restingBuyID, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest marketSellRequest = newOrderRequestHelper(marketSellID, 2, 0, 10, Side::Sell, OrderType::Market, 1001);
    OrderPtr restingBuy = engine.submit(restingBuyRequest).order;

    OrderPtr marketSell = engine.submit(marketSellRequest).order;

    EXPECT_EQ(marketSell->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_EQ(marketSell->getQty(), static_cast<Quantity>(5));
    EXPECT_FALSE(orderBook.doesOrderExist(marketSellID));
    EXPECT_EQ(restingBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(restingBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(restingBuyID));
}

// =====================================================================
// Market order price and time priority
// =====================================================================

TEST_F(MatchingEngineMatchTest, Submit_MarketBuyAcrossTwoAskLevels_TradesWithTheBestAskFirst) {
    OrderID worseSellID = 1;
    OrderID bestSellID = 2;
    OrderID marketBuyID = 3;
    NewOrderRequest worseSellRequest = newOrderRequestHelper(worseSellID, 1, 103, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest bestSellRequest = newOrderRequestHelper(bestSellID, 2, 101, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest marketBuyRequest = newOrderRequestHelper(marketBuyID, 3, 0, 10, Side::Buy, OrderType::Market, 1002);
    OrderPtr worseSell = engine.submit(worseSellRequest).order;
    OrderPtr bestSell = engine.submit(bestSellRequest).order;

    OrderPtr marketBuy = engine.submit(marketBuyRequest).order;

    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketBuyID));
    EXPECT_EQ(bestSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(bestSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(bestSellID));
    EXPECT_EQ(worseSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(worseSell->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(worseSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketSellAcrossTwoBidLevels_TradesWithTheBestBidFirst) {
    OrderID worseBuyID = 1;
    OrderID bestBuyID = 2;
    OrderID marketSellID = 3;
    NewOrderRequest worseBuyRequest = newOrderRequestHelper(worseBuyID, 1, 97, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest bestBuyRequest = newOrderRequestHelper(bestBuyID, 2, 99, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest marketSellRequest = newOrderRequestHelper(marketSellID, 3, 0, 10, Side::Sell, OrderType::Market, 1002);
    OrderPtr worseBuy = engine.submit(worseBuyRequest).order;
    OrderPtr bestBuy = engine.submit(bestBuyRequest).order;

    OrderPtr marketSell = engine.submit(marketSellRequest).order;

    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketSellID));
    EXPECT_EQ(bestBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(bestBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(bestBuyID));
    EXPECT_EQ(worseBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(worseBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(worseBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketBuyAgainstTwoSellsAtOnePrice_TradesWithTheEarlierSellFirst) {
    OrderID earlierSellID = 1;
    OrderID laterSellID = 2;
    OrderID marketBuyID = 3;
    NewOrderRequest earlierSellRequest = newOrderRequestHelper(earlierSellID, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest laterSellRequest = newOrderRequestHelper(laterSellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest marketBuyRequest = newOrderRequestHelper(marketBuyID, 3, 0, 10, Side::Buy, OrderType::Market, 1002);
    OrderPtr earlierSell = engine.submit(earlierSellRequest).order;
    OrderPtr laterSell = engine.submit(laterSellRequest).order;

    OrderPtr marketBuy = engine.submit(marketBuyRequest).order;

    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketBuyID));
    EXPECT_EQ(earlierSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(earlierSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(earlierSellID));
    EXPECT_EQ(laterSell->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(laterSell->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(laterSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketSellAgainstTwoBuysAtOnePrice_TradesWithTheEarlierBuyFirst) {
    OrderID earlierBuyID = 1;
    OrderID laterBuyID = 2;
    OrderID marketSellID = 3;
    NewOrderRequest earlierBuyRequest = newOrderRequestHelper(earlierBuyID, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest laterBuyRequest = newOrderRequestHelper(laterBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest marketSellRequest = newOrderRequestHelper(marketSellID, 3, 0, 10, Side::Sell, OrderType::Market, 1002);
    OrderPtr earlierBuy = engine.submit(earlierBuyRequest).order;
    OrderPtr laterBuy = engine.submit(laterBuyRequest).order;

    OrderPtr marketSell = engine.submit(marketSellRequest).order;

    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketSellID));
    EXPECT_EQ(earlierBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(earlierBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(earlierBuyID));
    EXPECT_EQ(laterBuy->getStatus(), OrderStatus::Pending);
    EXPECT_EQ(laterBuy->getQty(), static_cast<Quantity>(10));
    EXPECT_TRUE(orderBook.doesOrderExist(laterBuyID));
}

// =====================================================================
// Market order sweeps
// =====================================================================

TEST_F(MatchingEngineMatchTest, Submit_MarketBuySweepsTwoAskLevels_LeavesTheSecondLevelPartiallyExecuted) {
    OrderID firstSellID = 1;
    OrderID secondSellID = 2;
    OrderID marketBuyID = 3;
    NewOrderRequest firstSellRequest = newOrderRequestHelper(firstSellID, 1, 100, 50, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSellRequest = newOrderRequestHelper(secondSellID, 2, 102, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest marketBuyRequest = newOrderRequestHelper(marketBuyID, 3, 0, 55, Side::Buy, OrderType::Market, 1002);
    OrderPtr firstSell = engine.submit(firstSellRequest).order;
    OrderPtr secondSell = engine.submit(secondSellRequest).order;

    OrderPtr marketBuy = engine.submit(marketBuyRequest).order;

    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketBuyID));
    EXPECT_EQ(firstSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstSellID));
    EXPECT_EQ(secondSell->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(secondSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketSellSweepsTwoBidLevels_LeavesTheSecondLevelPartiallyExecuted) {
    OrderID firstBuyID = 1;
    OrderID secondBuyID = 2;
    OrderID marketSellID = 3;
    NewOrderRequest firstBuyRequest = newOrderRequestHelper(firstBuyID, 1, 100, 50, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuyRequest = newOrderRequestHelper(secondBuyID, 2, 98, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest marketSellRequest = newOrderRequestHelper(marketSellID, 3, 0, 55, Side::Sell, OrderType::Market, 1002);
    OrderPtr firstBuy = engine.submit(firstBuyRequest).order;
    OrderPtr secondBuy = engine.submit(secondBuyRequest).order;

    OrderPtr marketSell = engine.submit(marketSellRequest).order;

    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketSellID));
    EXPECT_EQ(firstBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstBuyID));
    EXPECT_EQ(secondBuy->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(secondBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondBuyID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketBuySweepsTwoSellsAtOnePrice_LeavesTheSecondSellPartiallyExecuted) {
    OrderID firstSellID = 1;
    OrderID secondSellID = 2;
    OrderID marketBuyID = 3;
    NewOrderRequest firstSellRequest = newOrderRequestHelper(firstSellID, 1, 100, 50, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondSellRequest = newOrderRequestHelper(secondSellID, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest marketBuyRequest = newOrderRequestHelper(marketBuyID, 3, 0, 55, Side::Buy, OrderType::Market, 1002);
    OrderPtr firstSell = engine.submit(firstSellRequest).order;
    OrderPtr secondSell = engine.submit(secondSellRequest).order;

    OrderPtr marketBuy = engine.submit(marketBuyRequest).order;

    EXPECT_EQ(marketBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketBuyID));
    EXPECT_EQ(firstSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstSellID));
    EXPECT_EQ(secondSell->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(secondSell->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondSellID));
}

TEST_F(MatchingEngineMatchTest, Submit_MarketSellSweepsTwoBuysAtOnePrice_LeavesTheSecondBuyPartiallyExecuted) {
    OrderID firstBuyID = 1;
    OrderID secondBuyID = 2;
    OrderID marketSellID = 3;
    NewOrderRequest firstBuyRequest = newOrderRequestHelper(firstBuyID, 1, 100, 50, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondBuyRequest = newOrderRequestHelper(secondBuyID, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest marketSellRequest = newOrderRequestHelper(marketSellID, 3, 0, 55, Side::Sell, OrderType::Market, 1002);
    OrderPtr firstBuy = engine.submit(firstBuyRequest).order;
    OrderPtr secondBuy = engine.submit(secondBuyRequest).order;

    OrderPtr marketSell = engine.submit(marketSellRequest).order;

    EXPECT_EQ(marketSell->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(marketSell->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(marketSellID));
    EXPECT_EQ(firstBuy->getStatus(), OrderStatus::Executed);
    EXPECT_EQ(firstBuy->getQty(), static_cast<Quantity>(0));
    EXPECT_FALSE(orderBook.doesOrderExist(firstBuyID));
    EXPECT_EQ(secondBuy->getStatus(), OrderStatus::PartiallyExecuted);
    EXPECT_EQ(secondBuy->getQty(), static_cast<Quantity>(5));
    EXPECT_TRUE(orderBook.doesOrderExist(secondBuyID));
}
