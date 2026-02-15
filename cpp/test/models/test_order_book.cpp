#include <gtest/gtest.h>
#include <memory>
#include "models/order_book.hpp"

class OrderBookTest : public ::testing::Test {
protected:
    LimitOrderBook book;
};

TEST_F(OrderBookTest, AddOrderSuccess) {
    OrderPtr order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);

    EXPECT_EQ(book.addOrder(order1), RejectionReason::None);
    EXPECT_TRUE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(order2), RejectionReason::None);
    EXPECT_TRUE(book.doesOrderExist(2));

    delete order1;
    delete order2;
}

TEST_F(OrderBookTest, NullOrderAdditionFails) {
    OrderPtr nullOrder = nullptr;

    EXPECT_EQ(book.addOrder(nullOrder), RejectionReason::NullOrder);
}

TEST_F(OrderBookTest, AddInvalidQtyOrderFails) {
    OrderPtr zeroQtyLimitOrder = new Order(1, 1, 100, 0, Side::Buy, OrderType::Limit, 1000);
    OrderPtr negativeQtyLimitOrder = new Order(2, 2, 100, -10, Side::Buy, OrderType::Limit, 1001);

    EXPECT_EQ(book.addOrder(zeroQtyLimitOrder), RejectionReason::InvalidQuantity);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(negativeQtyLimitOrder), RejectionReason::InvalidQuantity);
    EXPECT_FALSE(book.doesOrderExist(2));

    delete zeroQtyLimitOrder;
    delete negativeQtyLimitOrder;
}

TEST_F(OrderBookTest, AddMarketOrderFails) {
    OrderPtr marketOrder1 = new Order(1, 1, 0, 10, Side::Buy, OrderType::Market, 1000);
    OrderPtr marketOrder2 = new Order(2, 2, 0, 10, Side::Sell, OrderType::Market, 1001);

    EXPECT_EQ(book.addOrder(marketOrder1), RejectionReason::AddingMarketOrder);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(marketOrder2), RejectionReason::AddingMarketOrder);
    EXPECT_FALSE(book.doesOrderExist(2));

    delete marketOrder1;
    delete marketOrder2;
}

TEST_F(OrderBookTest, AddCancelOrderFails) {
    OrderPtr cancelOrder1 = new Order(1, 1, 0, 10, Side::Buy, OrderType::Cancel, 1000);
    OrderPtr cancelOrder2 = new Order(2, 2, 0, 10, Side::Sell, OrderType::Cancel, 1001);

    EXPECT_EQ(book.addOrder(cancelOrder1), RejectionReason::AddingCancelOrder);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(cancelOrder2), RejectionReason::AddingCancelOrder);
    EXPECT_FALSE(book.doesOrderExist(2));

    delete cancelOrder1;
    delete cancelOrder2;
}

TEST_F(OrderBookTest, AddOrderWithInvalidPriceFails) {
    OrderPtr invalidPriceOrder1 = new Order(1, 1, 0, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr invalidPriceOrder2 = new Order(2, 2, -10, 10, Side::Sell, OrderType::Limit, 1001);

    EXPECT_EQ(book.addOrder(invalidPriceOrder1), RejectionReason::InvalidPrice);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(invalidPriceOrder2), RejectionReason::InvalidPrice);
    EXPECT_FALSE(book.doesOrderExist(2));

    delete invalidPriceOrder1;
    delete invalidPriceOrder2;
}

TEST_F(OrderBookTest, AddCancelledOrderFails) {
    OrderPtr order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    order1->setStatus(OrderStatus::Cancelled);
    order2->setStatus(OrderStatus::CancelledAfterPartialExecution);

    EXPECT_EQ(book.addOrder(order1), RejectionReason::AddingCancelledOrder);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(order2), RejectionReason::AddingCancelledOrder);
    EXPECT_FALSE(book.doesOrderExist(2));

    delete order1;
    delete order2;
}

TEST_F(OrderBookTest, AddExecutedOrderFails) {
    OrderPtr order = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    order->setStatus(OrderStatus::Executed);

    EXPECT_EQ(book.addOrder(order), RejectionReason::AddingExecutedOrder);
    EXPECT_FALSE(book.doesOrderExist(1));

    delete order;
}

TEST_F(OrderBookTest, AddDuplicateOrderFails) {
    OrderPtr order = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);

    EXPECT_EQ(book.addOrder(order), RejectionReason::AddingDuplicateOrder);

    delete order;
}

TEST_F(OrderBookTest, CancelOrderSuccess) {
    OrderPtr order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(order1);
    book.addOrder(order2);
    order2->setStatus(OrderStatus::PartiallyExecuted);

    EXPECT_EQ(book.cancelOrder(1), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(order1->getStatus(), OrderStatus::Cancelled);

    EXPECT_EQ(book.cancelOrder(2), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(2));
    EXPECT_EQ(order2->getStatus(), OrderStatus::CancelledAfterPartialExecution);

    delete order1;
    delete order2;
}

TEST_F(OrderBookTest, CancelNonExistingOrderFails) {
    EXPECT_EQ(book.cancelOrder(1), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.cancelOrder(2), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_FALSE(book.doesOrderExist(2));
}

TEST_F(OrderBookTest, CancelCancelledOrderFails) {
    OrderPtr order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(order1);
    book.addOrder(order2);
    order1->setStatus(OrderStatus::Cancelled);
    order2->setStatus(OrderStatus::CancelledAfterPartialExecution);

    EXPECT_EQ(book.cancelOrder(1), RejectionReason::OrderBookInvariantViolation);
    EXPECT_EQ(book.cancelOrder(2), RejectionReason::OrderBookInvariantViolation);

    delete order1;
    delete order2;
}

TEST_F(OrderBookTest, CancelExecutedOrderFails) {
    OrderPtr order = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);
    order->setStatus(OrderStatus::Executed);

    EXPECT_EQ(book.cancelOrder(1), RejectionReason::OrderBookInvariantViolation);

    delete order;
}

TEST_F(OrderBookTest, DoubleCancelFails) {
    OrderPtr order = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);

    EXPECT_EQ(book.cancelOrder(1), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.cancelOrder(1), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_FALSE(book.doesOrderExist(1));

    delete order;
}

TEST_F(OrderBookTest, GetBestBidAsk) {
    OrderPtr buyOrder1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr buyOrder2 = new Order(2, 2, 105, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr sellOrder1 = new Order(3, 3, 110, 10, Side::Sell, OrderType::Limit, 1002);
    OrderPtr sellOrder2 = new Order(4, 4, 115, 10, Side::Sell, OrderType::Limit, 1003);

    book.addOrder(buyOrder1);
    book.addOrder(buyOrder2);
    book.addOrder(sellOrder1);
    book.addOrder(sellOrder2);

    EXPECT_EQ(book.getBestBid(), 105ull);
    EXPECT_EQ(book.getBestAsk(), 110ull);

    book.cancelOrder(2);
    book.cancelOrder(3);

    EXPECT_EQ(book.getBestBid(), 100ull);
    EXPECT_EQ(book.getBestAsk(), 115ull);

    book.cancelOrder(1);
    book.cancelOrder(4);

    EXPECT_EQ(book.getBestBid(), std::nullopt);
    EXPECT_EQ(book.getBestAsk(), std::nullopt);

    delete buyOrder1;
    delete buyOrder2;
    delete sellOrder1;
    delete sellOrder2;
}

TEST_F(OrderBookTest, LimitOrdersMarketableOnEmptyBook) {
    OrderPtr buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr sellOrder = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);

    EXPECT_FALSE(book.isOrderMarketable(buyOrder));
    EXPECT_FALSE(book.isOrderMarketable(sellOrder));

    delete buyOrder;
    delete sellOrder;
}

TEST_F(OrderBookTest, LimitOrdersMarketableWithExistingBidsAsks) {
    OrderPtr buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr sellOrder = new Order(2, 2, 110, 10, Side::Sell, OrderType::Limit, 1001);

    book.addOrder(buyOrder);
    book.addOrder(sellOrder);

    OrderPtr marketableBuy = new Order(3, 3, 115, 10, Side::Buy, OrderType::Limit, 1002);
    OrderPtr nonMarketableBuy = new Order(4, 4, 90, 10, Side::Buy, OrderType::Limit, 1003);
    OrderPtr marketableSell = new Order(5, 5, 95, 10, Side::Sell, OrderType::Limit, 1004);
    OrderPtr nonMarketableSell = new Order(6, 6, 120, 10, Side::Sell, OrderType::Limit, 1005);

    EXPECT_TRUE(book.isOrderMarketable(marketableBuy));
    EXPECT_FALSE(book.isOrderMarketable(nonMarketableBuy));
    EXPECT_TRUE(book.isOrderMarketable(marketableSell));
    EXPECT_FALSE(book.isOrderMarketable(nonMarketableSell));

    delete buyOrder;
    delete sellOrder;
    delete marketableBuy;
    delete nonMarketableBuy;
    delete marketableSell;
    delete nonMarketableSell;
}

TEST_F(OrderBookTest, MarketOrdersNotMarketableOnEmptyBook) {
    OrderPtr marketBuyOrder = new Order(1, 1, 0, 10, Side::Buy, OrderType::Market, 1000);
    OrderPtr marketSellOrder = new Order(2, 2, 0, 10, Side::Sell, OrderType::Market, 1001);

    EXPECT_FALSE(book.isOrderMarketable(marketBuyOrder));
    EXPECT_FALSE(book.isOrderMarketable(marketSellOrder));

    delete marketBuyOrder;
    delete marketSellOrder;
}

TEST_F(OrderBookTest, MarketOrdersMarketableWithExistingBidsAsks) {
    OrderPtr buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr sellOrder = new Order(2, 2, 110, 10, Side::Sell, OrderType::Limit, 1001);

    book.addOrder(buyOrder);
    book.addOrder(sellOrder);

    OrderPtr marketBuyOrder = new Order(3, 3, 0, 10, Side::Buy, OrderType::Market, 1002);
    OrderPtr marketSellOrder = new Order(4, 4, 0, 10, Side::Sell, OrderType::Market, 1003);

    EXPECT_TRUE(book.isOrderMarketable(marketBuyOrder));
    EXPECT_TRUE(book.isOrderMarketable(marketSellOrder));

    delete buyOrder;
    delete sellOrder;
    delete marketBuyOrder;
    delete marketSellOrder;
}

TEST_F(OrderBookTest, ZeroQtyOrderNotMarketable) {
    OrderPtr zeroQtyLimitOrder = new Order(1, 1, 100, 0, Side::Buy, OrderType::Limit, 1000);
    OrderPtr zeroQtyMarketOrder = new Order(2, 2, 0, 0, Side::Sell, OrderType::Market, 1001);

    EXPECT_FALSE(book.isOrderMarketable(zeroQtyLimitOrder));
    EXPECT_FALSE(book.isOrderMarketable(zeroQtyMarketOrder));

    delete zeroQtyLimitOrder;
    delete zeroQtyMarketOrder;
}

TEST_F(OrderBookTest, GetMatchedOrderEmptyBookReturnsNull) {
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), nullptr);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), nullptr);
}

TEST_F(OrderBookTest, GetMatchedOrderBuyReturnsBestAskAndFifo) {
    OrderPtr ask1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    OrderPtr ask2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    OrderPtr ask3 = new Order(3, 3, 105, 10, Side::Sell, OrderType::Limit, 1002);

    book.addOrder(ask1);
    book.addOrder(ask2);
    book.addOrder(ask3);
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), ask1);

    book.cancelOrder(1);
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), ask2);

    book.cancelOrder(2);
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), ask3);

    book.cancelOrder(3); 
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), nullptr);

    delete ask1;
    delete ask2;
    delete ask3;
}

TEST_F(OrderBookTest, GetMatchedOrderSellReturnsBestBidAndFifo) {
    OrderPtr bid1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = new Order(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr bid3 = new Order(3, 3, 95, 10, Side::Buy, OrderType::Limit, 1002);

    book.addOrder(bid1);
    book.addOrder(bid2);
    book.addOrder(bid3);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), bid1);

    book.cancelOrder(1);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), bid2);

    book.cancelOrder(2);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), bid3);

    book.cancelOrder(3);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), nullptr);

    delete bid1;
    delete bid2;
    delete bid3;
}

TEST_F(OrderBookTest, PopFrontEmptyBookNoOp) {
    EXPECT_NO_THROW(book.popFront(Side::Buy));
    EXPECT_NO_THROW(book.popFront(Side::Sell));
    EXPECT_FALSE(book.getBestBid().has_value());
    EXPECT_FALSE(book.getBestAsk().has_value());
}

TEST_F(OrderBookTest, PopFrontBuyRemovesFifoFromBestAsk) {
    OrderPtr ask1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    OrderPtr ask2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    OrderPtr ask3 = new Order(3, 3, 105, 10, Side::Sell, OrderType::Limit, 1002);

    book.addOrder(ask1);
    book.addOrder(ask2);
    book.addOrder(ask3);

    book.popFront(Side::Buy);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_TRUE(book.doesOrderExist(2));
    EXPECT_TRUE(book.doesOrderExist(3));
    EXPECT_EQ(book.getBestAsk(), 100ull);

    book.popFront(Side::Buy); 
    EXPECT_FALSE(book.doesOrderExist(2)); 
    EXPECT_TRUE(book.doesOrderExist(3)); 
    EXPECT_EQ(book.getBestAsk(), 105ull); 

    delete ask1;
    delete ask2;
    delete ask3;
}

TEST_F(OrderBookTest, PopFrontBuyRemovesEmptyAskLevel) {
    OrderPtr ask1 = new Order(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    OrderPtr ask2 = new Order(2, 2, 105, 10, Side::Sell, OrderType::Limit, 1001);

    book.addOrder(ask1);
    book.addOrder(ask2);

    book.popFront(Side::Buy);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_TRUE(book.doesOrderExist(2));
    EXPECT_EQ(book.getBestAsk(), 105ull);

    book.popFront(Side::Buy);
    EXPECT_FALSE(book.doesOrderExist(2));
    EXPECT_EQ(book.getBestAsk(), std::nullopt);

    delete ask1;
    delete ask2;
}

TEST_F(OrderBookTest, PopFrontSellRemovesFifoFromBestBid) {
    OrderPtr bid1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = new Order(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr bid3 = new Order(3, 3, 95, 10, Side::Buy, OrderType::Limit, 1002);

    book.addOrder(bid1);
    book.addOrder(bid2);
    book.addOrder(bid3);

    book.popFront(Side::Sell);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_TRUE(book.doesOrderExist(2));
    EXPECT_TRUE(book.doesOrderExist(3));
    EXPECT_EQ(book.getBestBid(), 100ull);

    book.popFront(Side::Sell);
    EXPECT_FALSE(book.doesOrderExist(2));
    EXPECT_TRUE(book.doesOrderExist(3));
    EXPECT_EQ(book.getBestBid(), 95ull);

    delete bid1;
    delete bid2;
    delete bid3;
}

TEST_F(OrderBookTest, PopFrontSellRemovesEmptyBidLevel) {
    OrderPtr bid1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = new Order(2, 2, 95, 10, Side::Buy, OrderType::Limit, 1001);

    book.addOrder(bid1);
    book.addOrder(bid2);

    book.popFront(Side::Sell);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_TRUE(book.doesOrderExist(2));
    EXPECT_EQ(book.getBestBid(), 95ull);

    book.popFront(Side::Sell);
    EXPECT_FALSE(book.doesOrderExist(2));
    EXPECT_EQ(book.getBestBid(), std::nullopt);

    delete bid1;
    delete bid2;
}

TEST_F(OrderBookTest, TradeExecutionCountInitiallyZero) {
    EXPECT_EQ(book.getTradeExecutionCount(), 0u);
}

TEST_F(OrderBookTest, TotalVolumeTradedInitiallyZero) {
    EXPECT_EQ(book.getTotalVolumeTraded(), 0u);
}

TEST_F(OrderBookTest, RecordExecutionIgnoresNonPositiveQty) {
    book.recordExecution(-5);

    EXPECT_EQ(book.getTradeExecutionCount(), 0u);
    EXPECT_EQ(book.getTotalVolumeTraded(), 0u);
}

TEST_F(OrderBookTest, RecordExecutionIncrementsCountsAndVolume) {
    book.recordExecution(10);
    EXPECT_EQ(book.getTradeExecutionCount(), 1u);
    EXPECT_EQ(book.getTotalVolumeTraded(), 10u);

    book.recordExecution(25);
    EXPECT_EQ(book.getTradeExecutionCount(), 2u);
    EXPECT_EQ(book.getTotalVolumeTraded(), 35u);
}

TEST_F(OrderBookTest, OrderCancellationCountInitiallyZero) {
    EXPECT_EQ(book.getOrderCancellationCount(), 0u);
}

TEST_F(OrderBookTest, RecordCancellationIncrementsCount) {
    book.recordCancellation();
    EXPECT_EQ(book.getOrderCancellationCount(), 1u);

    book.recordCancellation();
    EXPECT_EQ(book.getOrderCancellationCount(), 2u);
}

TEST_F(OrderBookTest, SnapshotEmptyBook) {
    auto snap = book.snapshot(1234);

    EXPECT_EQ(snap.timestamp, 1234u);
    EXPECT_EQ(snap.bestBid, std::nullopt);
    EXPECT_EQ(snap.bestAsk, std::nullopt);
    EXPECT_EQ(snap.spread, std::nullopt);
    EXPECT_EQ(snap.mid, std::nullopt);

    EXPECT_EQ(snap.bidSummary.totalQuantity, 0);
    EXPECT_EQ(snap.bidSummary.orderCount, 0u);
    EXPECT_EQ(snap.bidSummary.totalNotionalValue, 0u);

    EXPECT_EQ(snap.askSummary.totalQuantity, 0);
    EXPECT_EQ(snap.askSummary.orderCount, 0u);
    EXPECT_EQ(snap.askSummary.totalNotionalValue, 0u);

    EXPECT_TRUE(snap.bidDepths.empty());
    EXPECT_TRUE(snap.askDepths.empty());

    EXPECT_EQ(snap.tempo.tradeExecutionCount, 0u);
    EXPECT_EQ(snap.tempo.orderCancellationCount, 0u);
    EXPECT_EQ(snap.tempo.totalVolumeTraded, 0u);
}

TEST_F(OrderBookTest, SnapshotSingleBidLevelSingleBidOrder) {
    OrderPtr bid = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(bid);

    auto snap = book.snapshot(1234);

    EXPECT_EQ(snap.timestamp, 1234u);
    EXPECT_EQ(snap.bestBid, 100ull);
    EXPECT_EQ(snap.bestAsk, std::nullopt);
    EXPECT_EQ(snap.spread, std::nullopt);
    EXPECT_EQ(snap.mid, std::nullopt);

    EXPECT_EQ(snap.bidSummary.totalQuantity, 10);
    EXPECT_EQ(snap.bidSummary.orderCount, 1u);
    EXPECT_EQ(snap.bidSummary.totalNotionalValue, 1000u);

    EXPECT_EQ(snap.askSummary.totalQuantity, 0);
    EXPECT_EQ(snap.askSummary.orderCount, 0u);
    EXPECT_EQ(snap.askSummary.totalNotionalValue, 0u);

    ASSERT_EQ(snap.bidDepths.size(), 1u);
    EXPECT_EQ(snap.bidDepths[0].price, 100);
    EXPECT_EQ(snap.bidDepths[0].totalQuantity, 10);
    EXPECT_EQ(snap.bidDepths[0].orderCount, 1u);
    EXPECT_TRUE(snap.askDepths.empty());

    EXPECT_EQ(snap.tempo.tradeExecutionCount, 0u);
    EXPECT_EQ(snap.tempo.orderCancellationCount, 0u);
    EXPECT_EQ(snap.tempo.totalVolumeTraded, 0u);

    delete bid;
}

TEST_F(OrderBookTest, SnapshotSingleAskLevelSingleAskOrder) {
    OrderPtr ask = new Order(1, 1, 110, 7, Side::Sell, OrderType::Limit, 1000);
    book.addOrder(ask);

    auto snap = book.snapshot(1234);

    EXPECT_EQ(snap.timestamp, 1234u);
    EXPECT_EQ(snap.bestBid, std::nullopt);
    EXPECT_EQ(snap.bestAsk, 110ull);
    EXPECT_EQ(snap.spread, std::nullopt);
    EXPECT_EQ(snap.mid, std::nullopt);

    EXPECT_EQ(snap.bidSummary.totalQuantity, 0);
    EXPECT_EQ(snap.bidSummary.orderCount, 0u);
    EXPECT_EQ(snap.bidSummary.totalNotionalValue, 0u);

    EXPECT_EQ(snap.askSummary.totalQuantity, 7);
    EXPECT_EQ(snap.askSummary.orderCount, 1u);
    EXPECT_EQ(snap.askSummary.totalNotionalValue, 770u);

    EXPECT_TRUE(snap.bidDepths.empty());
    ASSERT_EQ(snap.askDepths.size(), 1u);
    EXPECT_EQ(snap.askDepths[0].price, 110);
    EXPECT_EQ(snap.askDepths[0].totalQuantity, 7);
    EXPECT_EQ(snap.askDepths[0].orderCount, 1u);

    EXPECT_EQ(snap.tempo.tradeExecutionCount, 0u);
    EXPECT_EQ(snap.tempo.orderCancellationCount, 0u);
    EXPECT_EQ(snap.tempo.totalVolumeTraded, 0u);

    delete ask;
}

TEST_F(OrderBookTest, SnapshotSingleBidOrderSingleAskOrder) {
    OrderPtr bid = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr ask = new Order(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(bid);
    book.addOrder(ask);

    auto snap = book.snapshot(1234);

    EXPECT_EQ(snap.timestamp, 1234u);
    EXPECT_EQ(snap.bestBid, 100ull);
    EXPECT_EQ(snap.bestAsk, 110ull);
    EXPECT_EQ(snap.spread, 10ull);
    EXPECT_EQ(snap.mid, 105ull);

    EXPECT_EQ(snap.bidSummary.totalQuantity, 10);
    EXPECT_EQ(snap.bidSummary.orderCount, 1u);
    EXPECT_EQ(snap.bidSummary.totalNotionalValue, 1000u);

    EXPECT_EQ(snap.askSummary.totalQuantity, 5);
    EXPECT_EQ(snap.askSummary.orderCount, 1u);
    EXPECT_EQ(snap.askSummary.totalNotionalValue, 550u);

    ASSERT_EQ(snap.bidDepths.size(), 1u);
    EXPECT_EQ(snap.bidDepths[0].price, 100);
    EXPECT_EQ(snap.bidDepths[0].totalQuantity, 10);
    EXPECT_EQ(snap.bidDepths[0].orderCount, 1u);

    ASSERT_EQ(snap.askDepths.size(), 1u);
    EXPECT_EQ(snap.askDepths[0].price, 110);
    EXPECT_EQ(snap.askDepths[0].totalQuantity, 5);
    EXPECT_EQ(snap.askDepths[0].orderCount, 1u);

    EXPECT_EQ(snap.tempo.tradeExecutionCount, 0u);
    EXPECT_EQ(snap.tempo.orderCancellationCount, 0u);
    EXPECT_EQ(snap.tempo.totalVolumeTraded, 0u);

    delete bid;
    delete ask;
}

TEST_F(OrderBookTest, SnapshotSingleBidLevelMultipleBidOrders) {
    OrderPtr bid1 = new Order(1, 1, 100, 3, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = new Order(2, 2, 100, 7, Side::Buy, OrderType::Limit, 1001);
    book.addOrder(bid1);
    book.addOrder(bid2);

    auto snap = book.snapshot(1234);

    EXPECT_EQ(snap.timestamp, 1234u);
    EXPECT_EQ(snap.bestBid, 100ull);
    EXPECT_EQ(snap.bestAsk, std::nullopt);
    EXPECT_EQ(snap.spread, std::nullopt);
    EXPECT_EQ(snap.mid, std::nullopt);

    EXPECT_EQ(snap.bidSummary.totalQuantity, 10);
    EXPECT_EQ(snap.bidSummary.orderCount, 2u);
    EXPECT_EQ(snap.bidSummary.totalNotionalValue, 1000u);

    EXPECT_EQ(snap.askSummary.totalQuantity, 0);
    EXPECT_EQ(snap.askSummary.orderCount, 0u);
    EXPECT_EQ(snap.askSummary.totalNotionalValue, 0u);

    ASSERT_EQ(snap.bidDepths.size(), 1u);
    EXPECT_EQ(snap.bidDepths[0].price, 100);
    EXPECT_EQ(snap.bidDepths[0].totalQuantity, 10);
    EXPECT_EQ(snap.bidDepths[0].orderCount, 2u);
    EXPECT_TRUE(snap.askDepths.empty());

    EXPECT_EQ(snap.tempo.tradeExecutionCount, 0u);
    EXPECT_EQ(snap.tempo.orderCancellationCount, 0u);
    EXPECT_EQ(snap.tempo.totalVolumeTraded, 0u);

    delete bid1;
    delete bid2;
}

TEST_F(OrderBookTest, SnapshotSingleAskLevelMultipleAskOrders) {
    OrderPtr ask1 = new Order(1, 1, 110, 4, Side::Sell, OrderType::Limit, 1000);
    OrderPtr ask2 = new Order(2, 2, 110, 6, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(ask1);
    book.addOrder(ask2);

    auto snap = book.snapshot(1234);

    EXPECT_EQ(snap.timestamp, 1234u);
    EXPECT_EQ(snap.bestBid, std::nullopt);
    EXPECT_EQ(snap.bestAsk, 110ull);
    EXPECT_EQ(snap.spread, std::nullopt);
    EXPECT_EQ(snap.mid, std::nullopt);

    EXPECT_EQ(snap.bidSummary.totalQuantity, 0);
    EXPECT_EQ(snap.bidSummary.orderCount, 0u);
    EXPECT_EQ(snap.bidSummary.totalNotionalValue, 0u);

    EXPECT_EQ(snap.askSummary.totalQuantity, 10);
    EXPECT_EQ(snap.askSummary.orderCount, 2u);
    EXPECT_EQ(snap.askSummary.totalNotionalValue, 1100u);

    EXPECT_TRUE(snap.bidDepths.empty());
    ASSERT_EQ(snap.askDepths.size(), 1u);
    EXPECT_EQ(snap.askDepths[0].price, 110);
    EXPECT_EQ(snap.askDepths[0].totalQuantity, 10);
    EXPECT_EQ(snap.askDepths[0].orderCount, 2u);

    EXPECT_EQ(snap.tempo.tradeExecutionCount, 0u);
    EXPECT_EQ(snap.tempo.orderCancellationCount, 0u);
    EXPECT_EQ(snap.tempo.totalVolumeTraded, 0u);

    delete ask1;
    delete ask2;
}

TEST_F(OrderBookTest, SnapshotMultipleBidAskLevels) {
    OrderPtr bid1 = new Order(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = new Order(2, 2, 105, 8, Side::Buy, OrderType::Limit, 1001);
    OrderPtr ask1 = new Order(3, 3, 110, 4, Side::Sell, OrderType::Limit, 1002);
    OrderPtr ask2 = new Order(4, 4, 108, 6, Side::Sell, OrderType::Limit, 1003);
    book.addOrder(bid1);
    book.addOrder(bid2);
    book.addOrder(ask1);
    book.addOrder(ask2);

    auto snap = book.snapshot(1234);

    EXPECT_EQ(snap.timestamp, 1234u);
    EXPECT_EQ(snap.bestBid, 105ull);
    EXPECT_EQ(snap.bestAsk, 108ull);
    EXPECT_EQ(snap.spread, 3ull);
    EXPECT_EQ(snap.mid, 106ull);

    EXPECT_EQ(snap.bidSummary.totalQuantity, 13);
    EXPECT_EQ(snap.bidSummary.orderCount, 2u);
    EXPECT_EQ(snap.bidSummary.totalNotionalValue, 1340u);

    EXPECT_EQ(snap.askSummary.totalQuantity, 10);
    EXPECT_EQ(snap.askSummary.orderCount, 2u);
    EXPECT_EQ(snap.askSummary.totalNotionalValue, 1088u);

    ASSERT_EQ(snap.bidDepths.size(), 2u);
    EXPECT_EQ(snap.bidDepths[0].price, 105);
    EXPECT_EQ(snap.bidDepths[1].price, 100);

    ASSERT_EQ(snap.askDepths.size(), 2u);
    EXPECT_EQ(snap.askDepths[0].price, 108);
    EXPECT_EQ(snap.askDepths[1].price, 110);

    EXPECT_EQ(snap.tempo.tradeExecutionCount, 0u);
    EXPECT_EQ(snap.tempo.orderCancellationCount, 0u);
    EXPECT_EQ(snap.tempo.totalVolumeTraded, 0u);

    delete bid1;
    delete bid2;
    delete ask1;
    delete ask2;
}

TEST_F(OrderBookTest, SnapshotTempoPropagation) {
    book.recordExecution(10);
    book.recordExecution(5);
    book.recordCancellation();
    book.recordCancellation();

    auto snap = book.snapshot(1234);

    EXPECT_EQ(snap.tempo.tradeExecutionCount, 2u);
    EXPECT_EQ(snap.tempo.orderCancellationCount, 2u);
    EXPECT_EQ(snap.tempo.totalVolumeTraded, 15u);
}