#include <gtest/gtest.h>
#include "engine/matching_engine.hpp"
#include "support/new_order.hpp"

class MatchingEngineExecutionTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    MatchingEngine engine{&orderBook, &stpPolicy};
};

// =====================================================================
// Trade count and volume
// =====================================================================

TEST_F(MatchingEngineExecutionTest, Submit_RestingBuyOnly_CountAndVolumeStayZero) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);

    engine.submit(restingBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(0));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(0));
}

TEST_F(MatchingEngineExecutionTest, Submit_RestingSellOnly_CountAndVolumeStayZero) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);

    engine.submit(restingSell);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(0));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(0));
}

TEST_F(MatchingEngineExecutionTest, Submit_BuyThenNonCrossingSell_CountAndVolumeStayZero) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 99, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest nonCrossingSell = newOrderRequestHelper(2, 2, 101, 10, Side::Sell, OrderType::Limit, 1001);

    engine.submit(restingBuy);
    engine.submit(nonCrossingSell);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(0));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(0));
}

TEST_F(MatchingEngineExecutionTest, Submit_SellThenNonCrossingBuy_CountAndVolumeStayZero) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 101, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest nonCrossingBuy = newOrderRequestHelper(2, 2, 99, 10, Side::Buy, OrderType::Limit, 1001);

    engine.submit(restingSell);
    engine.submit(nonCrossingBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(0));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(0));
}

TEST_F(MatchingEngineExecutionTest, Submit_BuyFullyFillsRestingSell_CountsOneTradeWithFullVolume) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest takerBuy = newOrderRequestHelper(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    engine.submit(restingSell);

    engine.submit(takerBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(1));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(10));
}

TEST_F(MatchingEngineExecutionTest, Submit_SellFullyFillsRestingBuy_CountsOneTradeWithFullVolume) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest takerSell = newOrderRequestHelper(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    engine.submit(restingBuy);

    engine.submit(takerSell);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(1));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(10));
}

TEST_F(MatchingEngineExecutionTest, Submit_BuySweepsTwoRestingSells_CountsTwoTradesWithCombinedVolume) {
    NewOrderRequest firstRestingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondRestingSell = newOrderRequestHelper(2, 2, 101, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest takerBuy = newOrderRequestHelper(3, 3, 101, 12, Side::Buy, OrderType::Limit, 1002);
    engine.submit(firstRestingSell);
    engine.submit(secondRestingSell);

    engine.submit(takerBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(2));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(12));
}

TEST_F(MatchingEngineExecutionTest, Submit_SellSweepsTwoRestingBuys_CountsTwoTradesWithCombinedVolume) {
    NewOrderRequest firstRestingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondRestingBuy = newOrderRequestHelper(2, 2, 99, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest takerSell = newOrderRequestHelper(3, 3, 99, 12, Side::Sell, OrderType::Limit, 1002);
    engine.submit(firstRestingBuy);
    engine.submit(secondRestingBuy);

    engine.submit(takerSell);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(2));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(12));
}

TEST_F(MatchingEngineExecutionTest, Submit_BuyLargerThanRestingSell_CountsOnlyTheFilledVolume) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest takerBuy = newOrderRequestHelper(2, 2, 100, 15, Side::Buy, OrderType::Limit, 1001);
    engine.submit(restingSell);

    engine.submit(takerBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(1));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(10));
}

TEST_F(MatchingEngineExecutionTest, Submit_SellLargerThanRestingBuy_CountsOnlyTheFilledVolume) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest takerSell = newOrderRequestHelper(2, 2, 100, 15, Side::Sell, OrderType::Limit, 1001);
    engine.submit(restingBuy);

    engine.submit(takerSell);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(1));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(10));
}

TEST_F(MatchingEngineExecutionTest, Submit_TwoTakersFillOneRestingSell_CountAndVolumeAccumulate) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest firstTakerBuy = newOrderRequestHelper(2, 2, 100, 4, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest secondTakerBuy = newOrderRequestHelper(3, 3, 100, 6, Side::Buy, OrderType::Limit, 1002);
    engine.submit(restingSell);

    engine.submit(firstTakerBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(1));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(4));

    engine.submit(secondTakerBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(2));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(10));
}

// =====================================================================
// Trade count and volume for market orders
// =====================================================================

TEST_F(MatchingEngineExecutionTest, Submit_MarketBuyFillsRestingSell_CountsOneTradeWithFullVolume) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest marketBuy = newOrderRequestHelper(2, 2, 0, 10, Side::Buy, OrderType::Market, 1001);
    engine.submit(restingSell);

    engine.submit(marketBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(1));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(10));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketSellFillsRestingBuy_CountsOneTradeWithFullVolume) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest marketSell = newOrderRequestHelper(2, 2, 0, 10, Side::Sell, OrderType::Market, 1001);
    engine.submit(restingBuy);

    engine.submit(marketSell);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(1));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(10));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketBuyLargerThanRestingSell_CountsOnlyTheFilledVolume) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest marketBuy = newOrderRequestHelper(2, 2, 0, 8, Side::Buy, OrderType::Market, 1001);
    engine.submit(restingSell);

    engine.submit(marketBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(1));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(5));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketSellLargerThanRestingBuy_CountsOnlyTheFilledVolume) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest marketSell = newOrderRequestHelper(2, 2, 0, 8, Side::Sell, OrderType::Market, 1001);
    engine.submit(restingBuy);

    engine.submit(marketSell);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(1));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(5));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketBuySweepsTwoRestingSells_CountsEachTrade) {
    NewOrderRequest firstRestingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondRestingSell = newOrderRequestHelper(2, 2, 101, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest marketBuy = newOrderRequestHelper(3, 3, 0, 8, Side::Buy, OrderType::Market, 1002);
    engine.submit(firstRestingSell);
    engine.submit(secondRestingSell);

    engine.submit(marketBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(2));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(8));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketSellSweepsTwoRestingBuys_CountsEachTrade) {
    NewOrderRequest firstRestingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondRestingBuy = newOrderRequestHelper(2, 2, 99, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest marketSell = newOrderRequestHelper(3, 3, 0, 8, Side::Sell, OrderType::Market, 1002);
    engine.submit(firstRestingBuy);
    engine.submit(secondRestingBuy);

    engine.submit(marketSell);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(2));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(8));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketBuyWithNoLiquidity_CountAndVolumeStayZero) {
    NewOrderRequest marketBuy = newOrderRequestHelper(1, 1, 0, 10, Side::Buy, OrderType::Market, 1000);

    engine.submit(marketBuy);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(0));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(0));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketSellWithNoLiquidity_CountAndVolumeStayZero) {
    NewOrderRequest marketSell = newOrderRequestHelper(1, 1, 0, 10, Side::Sell, OrderType::Market, 1000);

    engine.submit(marketSell);

    EXPECT_EQ(orderBook.getTradeExecutionCount(), static_cast<Count>(0));
    EXPECT_EQ(orderBook.getTotalVolumeTraded(), static_cast<AggregateQuantity>(0));
}

// =====================================================================
// Last traded price
// =====================================================================

TEST_F(MatchingEngineExecutionTest, GetLastTradedPrice_NoTrades_IsEmpty) {
    EXPECT_EQ(engine.getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, Submit_RestingBuy_LastTradedPriceStaysEmpty) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);

    engine.submit(restingBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, Submit_RestingSell_LastTradedPriceStaysEmpty) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);

    engine.submit(restingSell);

    EXPECT_EQ(engine.getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, Submit_BuyFillsRestingSell_SetsLastTradedPrice) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest takerBuy = newOrderRequestHelper(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    engine.submit(restingSell);

    engine.submit(takerBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_SellFillsRestingBuy_SetsLastTradedPrice) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest takerSell = newOrderRequestHelper(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    engine.submit(restingBuy);

    engine.submit(takerSell);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_BuyTradesAtHigherPriceAfterEarlierTrade_LastTradedPriceIsTheLatest) {
    NewOrderRequest firstRestingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest firstTakerBuy = newOrderRequestHelper(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest secondRestingSell = newOrderRequestHelper(3, 3, 110, 5, Side::Sell, OrderType::Limit, 1002);
    NewOrderRequest secondTakerBuy = newOrderRequestHelper(4, 4, 110, 5, Side::Buy, OrderType::Limit, 1003);
    engine.submit(firstRestingSell);
    engine.submit(firstTakerBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));

    engine.submit(secondRestingSell);
    engine.submit(secondTakerBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(110));
}

TEST_F(MatchingEngineExecutionTest, Submit_SellTradesAtLowerPriceAfterEarlierTrade_LastTradedPriceIsTheLatest) {
    NewOrderRequest firstRestingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest firstTakerSell = newOrderRequestHelper(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest secondRestingBuy = newOrderRequestHelper(3, 3, 90, 5, Side::Buy, OrderType::Limit, 1002);
    NewOrderRequest secondTakerSell = newOrderRequestHelper(4, 4, 90, 5, Side::Sell, OrderType::Limit, 1003);
    engine.submit(firstRestingBuy);
    engine.submit(firstTakerSell);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));

    engine.submit(secondRestingBuy);
    engine.submit(secondTakerSell);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(90));
}

TEST_F(MatchingEngineExecutionTest, Submit_BuyPricedAboveRestingSell_LastTradedPriceIsMakerPrice) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest takerBuy = newOrderRequestHelper(2, 2, 105, 10, Side::Buy, OrderType::Limit, 1001);
    engine.submit(restingSell);

    engine.submit(takerBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_SellPricedBelowRestingBuy_LastTradedPriceIsMakerPrice) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest takerSell = newOrderRequestHelper(2, 2, 95, 10, Side::Sell, OrderType::Limit, 1001);
    engine.submit(restingBuy);

    engine.submit(takerSell);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_BuySweepsTwoLevels_LastTradedPriceIsTheLastLevel) {
    NewOrderRequest firstRestingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondRestingSell = newOrderRequestHelper(2, 2, 105, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest takerBuy = newOrderRequestHelper(3, 3, 110, 10, Side::Buy, OrderType::Limit, 1002);
    engine.submit(firstRestingSell);
    engine.submit(secondRestingSell);

    engine.submit(takerBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(105));
}

TEST_F(MatchingEngineExecutionTest, Submit_SellSweepsTwoLevels_LastTradedPriceIsTheLastLevel) {
    NewOrderRequest firstRestingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondRestingBuy = newOrderRequestHelper(2, 2, 95, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest takerSell = newOrderRequestHelper(3, 3, 90, 10, Side::Sell, OrderType::Limit, 1002);
    engine.submit(firstRestingBuy);
    engine.submit(secondRestingBuy);

    engine.submit(takerSell);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(95));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketBuyFillsRestingSell_LastTradedPriceIsMakerPrice) {
    NewOrderRequest restingSell = newOrderRequestHelper(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest marketBuy = newOrderRequestHelper(2, 2, 0, 10, Side::Buy, OrderType::Market, 1001);
    engine.submit(restingSell);

    engine.submit(marketBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketSellFillsRestingBuy_LastTradedPriceIsMakerPrice) {
    NewOrderRequest restingBuy = newOrderRequestHelper(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest marketSell = newOrderRequestHelper(2, 2, 0, 10, Side::Sell, OrderType::Market, 1001);
    engine.submit(restingBuy);

    engine.submit(marketSell);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketBuySweepsTwoLevels_LastTradedPriceIsTheLastLevel) {
    NewOrderRequest firstRestingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest secondRestingSell = newOrderRequestHelper(2, 2, 105, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest marketBuy = newOrderRequestHelper(3, 3, 0, 10, Side::Buy, OrderType::Market, 1002);
    engine.submit(firstRestingSell);
    engine.submit(secondRestingSell);

    engine.submit(marketBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(105));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketSellSweepsTwoLevels_LastTradedPriceIsTheLastLevel) {
    NewOrderRequest firstRestingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest secondRestingBuy = newOrderRequestHelper(2, 2, 95, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest marketSell = newOrderRequestHelper(3, 3, 0, 10, Side::Sell, OrderType::Market, 1002);
    engine.submit(firstRestingBuy);
    engine.submit(secondRestingBuy);

    engine.submit(marketSell);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(95));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketBuyWithNoLiquidity_LastTradedPriceStaysEmpty) {
    NewOrderRequest marketBuy = newOrderRequestHelper(1, 1, 0, 10, Side::Buy, OrderType::Market, 1000);

    engine.submit(marketBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketSellWithNoLiquidity_LastTradedPriceStaysEmpty) {
    NewOrderRequest marketSell = newOrderRequestHelper(1, 1, 0, 10, Side::Sell, OrderType::Market, 1000);

    engine.submit(marketSell);

    EXPECT_EQ(engine.getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketBuyWithNoLiquidityAfterEarlierTrade_LastTradedPriceStaysUnchanged) {
    NewOrderRequest earlierRestingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest earlierTakerBuy = newOrderRequestHelper(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    NewOrderRequest marketBuy = newOrderRequestHelper(3, 3, 0, 10, Side::Buy, OrderType::Market, 1002);
    engine.submit(earlierRestingSell);
    engine.submit(earlierTakerBuy);

    engine.submit(marketBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_MarketSellWithNoLiquidityAfterEarlierTrade_LastTradedPriceStaysUnchanged) {
    NewOrderRequest earlierRestingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest earlierTakerSell = newOrderRequestHelper(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1001);
    NewOrderRequest marketSell = newOrderRequestHelper(3, 3, 0, 10, Side::Sell, OrderType::Market, 1002);
    engine.submit(earlierRestingBuy);
    engine.submit(earlierTakerSell);

    engine.submit(marketSell);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_SelfTradeBuyAgainstRestingSell_LastTradedPriceStaysEmpty) {
    OwnerID sameOwnerID = 1;
    NewOrderRequest restingSell = newOrderRequestHelper(1, sameOwnerID, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest selfTradeBuy = newOrderRequestHelper(2, sameOwnerID, 100, 10, Side::Buy, OrderType::Limit, 1001);
    engine.submit(restingSell);

    engine.submit(selfTradeBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, Submit_SelfTradeSellAgainstRestingBuy_LastTradedPriceStaysEmpty) {
    OwnerID sameOwnerID = 1;
    NewOrderRequest restingBuy = newOrderRequestHelper(1, sameOwnerID, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest selfTradeSell = newOrderRequestHelper(2, sameOwnerID, 100, 10, Side::Sell, OrderType::Limit, 1001);
    engine.submit(restingBuy);

    engine.submit(selfTradeSell);

    EXPECT_EQ(engine.getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, Submit_SelfTradeBuyAfterEarlierTrade_LastTradedPriceStaysUnchanged) {
    NewOrderRequest earlierRestingSell = newOrderRequestHelper(1, 1, 100, 5, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest earlierTakerBuy = newOrderRequestHelper(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    OwnerID sameOwnerID = 3;
    NewOrderRequest restingSell = newOrderRequestHelper(3, sameOwnerID, 110, 10, Side::Sell, OrderType::Limit, 1002);
    NewOrderRequest selfTradeBuy = newOrderRequestHelper(4, sameOwnerID, 110, 10, Side::Buy, OrderType::Limit, 1003);
    engine.submit(earlierRestingSell);
    engine.submit(earlierTakerBuy);
    engine.submit(restingSell);

    engine.submit(selfTradeBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_SelfTradeSellAfterEarlierTrade_LastTradedPriceStaysUnchanged) {
    NewOrderRequest earlierRestingBuy = newOrderRequestHelper(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest earlierTakerSell = newOrderRequestHelper(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1001);
    OwnerID sameOwnerID = 3;
    NewOrderRequest restingBuy = newOrderRequestHelper(3, sameOwnerID, 90, 10, Side::Buy, OrderType::Limit, 1002);
    NewOrderRequest selfTradeSell = newOrderRequestHelper(4, sameOwnerID, 90, 10, Side::Sell, OrderType::Limit, 1003);
    engine.submit(earlierRestingBuy);
    engine.submit(earlierTakerSell);
    engine.submit(restingBuy);

    engine.submit(selfTradeSell);

    EXPECT_EQ(engine.getLastTradedPrice(), static_cast<PriceTicks>(100));
}

TEST_F(MatchingEngineExecutionTest, Submit_SelfTradeMarketBuyAgainstRestingSell_LastTradedPriceStaysEmpty) {
    OwnerID sameOwnerID = 1;
    NewOrderRequest restingSell = newOrderRequestHelper(1, sameOwnerID, 100, 10, Side::Sell, OrderType::Limit, 1000);
    NewOrderRequest selfTradeMarketBuy = newOrderRequestHelper(2, sameOwnerID, 0, 10, Side::Buy, OrderType::Market, 1001);
    engine.submit(restingSell);

    engine.submit(selfTradeMarketBuy);

    EXPECT_EQ(engine.getLastTradedPrice(), std::nullopt);
}

TEST_F(MatchingEngineExecutionTest, Submit_SelfTradeMarketSellAgainstRestingBuy_LastTradedPriceStaysEmpty) {
    OwnerID sameOwnerID = 1;
    NewOrderRequest restingBuy = newOrderRequestHelper(1, sameOwnerID, 100, 10, Side::Buy, OrderType::Limit, 1000);
    NewOrderRequest selfTradeMarketSell = newOrderRequestHelper(2, sameOwnerID, 0, 10, Side::Sell, OrderType::Market, 1001);
    engine.submit(restingBuy);

    engine.submit(selfTradeMarketSell);

    EXPECT_EQ(engine.getLastTradedPrice(), std::nullopt);
}
