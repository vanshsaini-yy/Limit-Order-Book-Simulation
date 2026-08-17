#include <gtest/gtest.h>
#include <memory>
#include "engine/limit_order_book.hpp"

class OrderBookTest : public ::testing::Test {
protected:
    LimitOrderBook book;
};

TEST_F(OrderBookTest, AddOrderSuccess) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);

    EXPECT_EQ(book.addOrder(order1), RejectionReason::None);
    EXPECT_TRUE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(order2), RejectionReason::None);
    EXPECT_TRUE(book.doesOrderExist(2));
}

TEST_F(OrderBookTest, NullOrderAdditionViolatesInvariant) {
    OrderPtr nullOrder = nullptr;

    EXPECT_EQ(book.addOrder(nullOrder), RejectionReason::OrderBookInvariantViolation);
}

TEST_F(OrderBookTest, AddInvalidQtyOrderViolatesInvariant) {
    OrderPtr zeroQtyLimitOrder = std::make_shared<Order>(1, 1, 100, 0, Side::Buy, OrderType::Limit, 1000);
    OrderPtr negativeQtyLimitOrder = std::make_shared<Order>(2, 2, 100, -10, Side::Buy, OrderType::Limit, 1001);

    EXPECT_EQ(book.addOrder(zeroQtyLimitOrder), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(negativeQtyLimitOrder), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(2));
}

TEST_F(OrderBookTest, AddMarketOrderViolatesInvariant) {
    OrderPtr marketOrder1 = std::make_shared<Order>(1, 1, 0, 10, Side::Buy, OrderType::Market, 1000);
    OrderPtr marketOrder2 = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1001);

    EXPECT_EQ(book.addOrder(marketOrder1), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(marketOrder2), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(2));
}

TEST_F(OrderBookTest, AddCancelOrderViolatesInvariant) {
    OrderPtr cancelOrder1 = std::make_shared<Order>(1, 1, 0, 10, Side::Buy, OrderType::Cancel, 1000);
    OrderPtr cancelOrder2 = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Cancel, 1001);

    EXPECT_EQ(book.addOrder(cancelOrder1), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(cancelOrder2), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(2));
}

TEST_F(OrderBookTest, AddLimitOrderWithInvalidPriceViolatesInvariant) {
    OrderPtr invalidPriceOrder1 = std::make_shared<Order>(1, 1, 0, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr invalidPriceOrder2 = std::make_shared<Order>(2, 2, -10, 10, Side::Sell, OrderType::Limit, 1001);

    EXPECT_EQ(book.addOrder(invalidPriceOrder1), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(invalidPriceOrder2), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(2));
}

TEST_F(OrderBookTest, AddCancelledOrderViolatesInvariant) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    order1->setStatus(OrderStatus::Cancelled);
    order2->setStatus(OrderStatus::CancelledAfterPartialExecution);

    EXPECT_EQ(book.addOrder(order1), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.addOrder(order2), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(2));
}

TEST_F(OrderBookTest, AddExecutedOrderViolatesInvariant) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    order->setStatus(OrderStatus::Executed);

    EXPECT_EQ(book.addOrder(order), RejectionReason::OrderBookInvariantViolation);
    EXPECT_FALSE(book.doesOrderExist(1));
}

TEST_F(OrderBookTest, AddSameOrderTwiceFails) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);

    EXPECT_EQ(book.addOrder(order), RejectionReason::DuplicateOrderID);
}

TEST_F(OrderBookTest, AddDistinctOrderWithDuplicateIDFails) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = std::make_shared<Order>(1, 2, 105, 5, Side::Buy, OrderType::Limit, 1001);
    book.addOrder(order1);

    EXPECT_EQ(book.addOrder(order2), RejectionReason::DuplicateOrderID);
}

TEST_F(OrderBookTest, CancelOrderSuccess) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(order1);
    book.addOrder(order2);
    order2->setStatus(OrderStatus::PartiallyExecuted);

    EXPECT_EQ(book.cancelOrder(1, 1), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(order1->getStatus(), OrderStatus::Cancelled);

    EXPECT_EQ(book.cancelOrder(2, 2), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(2));
    EXPECT_EQ(order2->getStatus(), OrderStatus::CancelledAfterPartialExecution);
}

TEST_F(OrderBookTest, CancelNonExistingOrderFails) {
    EXPECT_EQ(book.cancelOrder(1, 1), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.cancelOrder(2, 2), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_FALSE(book.doesOrderExist(2));
}

TEST_F(OrderBookTest, CancelCancelledOrderFails) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(order1);
    book.addOrder(order2);
    order1->setStatus(OrderStatus::Cancelled);
    order2->setStatus(OrderStatus::CancelledAfterPartialExecution);

    EXPECT_EQ(book.cancelOrder(1, 1), RejectionReason::OrderBookInvariantViolation);
    EXPECT_EQ(book.cancelOrder(2, 2), RejectionReason::OrderBookInvariantViolation);
}

TEST_F(OrderBookTest, CancelExecutedOrderFails) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);
    order->setStatus(OrderStatus::Executed);

    EXPECT_EQ(book.cancelOrder(1, 1), RejectionReason::OrderBookInvariantViolation);
}

TEST_F(OrderBookTest, DoubleCancelFails) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);

    EXPECT_EQ(book.cancelOrder(1, 1), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.cancelOrder(1, 1), RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_FALSE(book.doesOrderExist(1));
}

TEST_F(OrderBookTest, CancelOrderWithMatchingOwnerIDSucceeds) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);

    EXPECT_EQ(book.cancelOrder(1, 1), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(order->getStatus(), OrderStatus::Cancelled);
}

TEST_F(OrderBookTest, CancelOrderWithMismatchedOwnerIDFails) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);

    EXPECT_EQ(book.cancelOrder(1, 2), RejectionReason::OrderToBeCancelledDoesNotExist);
}

TEST_F(OrderBookTest, CancelOrderWithMismatchedOwnerIDDoesNotModifyOrder) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);

    book.cancelOrder(1, 2);

    EXPECT_EQ(order->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(book.doesOrderExist(1));
}

TEST_F(OrderBookTest, CancelNonExistingOrderWithOwnerIDFails) {
    EXPECT_EQ(book.cancelOrder(999, 1), RejectionReason::OrderToBeCancelledDoesNotExist);
}

TEST_F(OrderBookTest, CancelPartiallyExecutedOrderWithMatchingOwnerIDSucceeds) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    book.addOrder(order);
    order->setStatus(OrderStatus::PartiallyExecuted);

    EXPECT_EQ(book.cancelOrder(1, 1), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(order->getStatus(), OrderStatus::CancelledAfterPartialExecution);
}

TEST_F(OrderBookTest, GetBestBidAsk) {
    OrderPtr buyOrder1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr buyOrder2 = std::make_shared<Order>(2, 2, 105, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr sellOrder1 = std::make_shared<Order>(3, 3, 110, 10, Side::Sell, OrderType::Limit, 1002);
    OrderPtr sellOrder2 = std::make_shared<Order>(4, 4, 115, 10, Side::Sell, OrderType::Limit, 1003);

    book.addOrder(buyOrder1);
    book.addOrder(buyOrder2);
    book.addOrder(sellOrder1);
    book.addOrder(sellOrder2);

    EXPECT_EQ(book.getBestBid(), std::optional<PriceTicks>{105});
    EXPECT_EQ(book.getBestAsk(), std::optional<PriceTicks>{110});

    book.cancelOrder(2, 2);
    book.cancelOrder(3, 3);

    EXPECT_EQ(book.getBestBid(), std::optional<PriceTicks>{100});
    EXPECT_EQ(book.getBestAsk(), std::optional<PriceTicks>{115});

    book.cancelOrder(1, 1);
    book.cancelOrder(4, 4);

    EXPECT_EQ(book.getBestBid(), std::nullopt);
    EXPECT_EQ(book.getBestAsk(), std::nullopt);
}

TEST_F(OrderBookTest, GetMidPriceEmptyBookReturnsNullopt) {
    EXPECT_EQ(book.getMidPrice(), std::nullopt);
}

TEST_F(OrderBookTest, GetMidPriceOneSidedBookReturnsNullopt) {
    OrderPtr bid = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(bid);

    EXPECT_EQ(book.getMidPrice(), std::nullopt);
}

TEST_F(OrderBookTest, GetMidPriceBothSidesPresent) {
    OrderPtr bid = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr ask = std::make_shared<Order>(2, 2, 110, 10, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(bid);
    book.addOrder(ask);

    EXPECT_EQ(book.getMidPrice(), 105ull);
}

TEST_F(OrderBookTest, GetSpreadEmptyBookReturnsNullopt) {
    EXPECT_EQ(book.getSpread(), std::nullopt);
}

TEST_F(OrderBookTest, GetSpreadOneSidedBookReturnsNullopt) {
    OrderPtr ask = std::make_shared<Order>(1, 1, 110, 10, Side::Sell, OrderType::Limit, 1000);
    book.addOrder(ask);

    EXPECT_EQ(book.getSpread(), std::nullopt);
}

TEST_F(OrderBookTest, GetSpreadBothSidesPresent) {
    OrderPtr bid = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr ask = std::make_shared<Order>(2, 2, 110, 10, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(bid);
    book.addOrder(ask);

    EXPECT_EQ(book.getSpread(), 10ull);
}

TEST_F(OrderBookTest, LimitOrdersMarketableOnEmptyBook) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);

    EXPECT_FALSE(book.isOrderMarketable(buyOrder));
    EXPECT_FALSE(book.isOrderMarketable(sellOrder));
}

TEST_F(OrderBookTest, LimitOrdersMarketableWithExistingBidsAsks) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 110, 10, Side::Sell, OrderType::Limit, 1001);

    book.addOrder(buyOrder);
    book.addOrder(sellOrder);

    OrderPtr marketableBuy = std::make_shared<Order>(3, 3, 115, 10, Side::Buy, OrderType::Limit, 1002);
    OrderPtr nonMarketableBuy = std::make_shared<Order>(4, 4, 90, 10, Side::Buy, OrderType::Limit, 1003);
    OrderPtr marketableSell = std::make_shared<Order>(5, 5, 95, 10, Side::Sell, OrderType::Limit, 1004);
    OrderPtr nonMarketableSell = std::make_shared<Order>(6, 6, 120, 10, Side::Sell, OrderType::Limit, 1005);

    EXPECT_TRUE(book.isOrderMarketable(marketableBuy));
    EXPECT_FALSE(book.isOrderMarketable(nonMarketableBuy));
    EXPECT_TRUE(book.isOrderMarketable(marketableSell));
    EXPECT_FALSE(book.isOrderMarketable(nonMarketableSell));
}

TEST_F(OrderBookTest, MarketOrdersNotMarketableOnEmptyBook) {
    OrderPtr marketBuyOrder = std::make_shared<Order>(1, 1, 0, 10, Side::Buy, OrderType::Market, 1000);
    OrderPtr marketSellOrder = std::make_shared<Order>(2, 2, 0, 10, Side::Sell, OrderType::Market, 1001);

    EXPECT_FALSE(book.isOrderMarketable(marketBuyOrder));
    EXPECT_FALSE(book.isOrderMarketable(marketSellOrder));
}

TEST_F(OrderBookTest, MarketOrdersMarketableWithExistingBidsAsks) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 110, 10, Side::Sell, OrderType::Limit, 1001);

    book.addOrder(buyOrder);
    book.addOrder(sellOrder);

    OrderPtr marketBuyOrder = std::make_shared<Order>(3, 3, 0, 10, Side::Buy, OrderType::Market, 1002);
    OrderPtr marketSellOrder = std::make_shared<Order>(4, 4, 0, 10, Side::Sell, OrderType::Market, 1003);

    EXPECT_TRUE(book.isOrderMarketable(marketBuyOrder));
    EXPECT_TRUE(book.isOrderMarketable(marketSellOrder));
}

TEST_F(OrderBookTest, ZeroQtyOrderNotMarketable) {
    OrderPtr zeroQtyLimitOrder = std::make_shared<Order>(1, 1, 100, 0, Side::Buy, OrderType::Limit, 1000);
    OrderPtr zeroQtyMarketOrder = std::make_shared<Order>(2, 2, 0, 0, Side::Sell, OrderType::Market, 1001);

    EXPECT_FALSE(book.isOrderMarketable(zeroQtyLimitOrder));
    EXPECT_FALSE(book.isOrderMarketable(zeroQtyMarketOrder));
}

TEST_F(OrderBookTest, CancelTypeOrderNotMarketable) {
    OrderPtr buyOrder = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr sellOrder = std::make_shared<Order>(2, 2, 110, 10, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(buyOrder);
    book.addOrder(sellOrder);

    OrderPtr cancelOrder = std::make_shared<Order>(3, 3, 0, 0, Side::None, OrderType::Cancel, 1002);
    EXPECT_FALSE(book.isOrderMarketable(cancelOrder));
}

TEST_F(OrderBookTest, GetMatchedOrderEmptyBookReturnsNull) {
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), nullptr);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), nullptr);
}

TEST_F(OrderBookTest, GetMatchedOrderBuyReturnsBestAskAndFifo) {
    OrderPtr ask1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    OrderPtr ask2 = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    OrderPtr ask3 = std::make_shared<Order>(3, 3, 105, 10, Side::Sell, OrderType::Limit, 1002);

    book.addOrder(ask1);
    book.addOrder(ask2);
    book.addOrder(ask3);
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), ask1);

    book.cancelOrder(1, 1);
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), ask2);

    book.cancelOrder(2, 2);
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), ask3);

    book.cancelOrder(3, 3);
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), nullptr);
}

TEST_F(OrderBookTest, GetMatchedOrderSellReturnsBestBidAndFifo) {
    OrderPtr bid1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr bid3 = std::make_shared<Order>(3, 3, 95, 10, Side::Buy, OrderType::Limit, 1002);

    book.addOrder(bid1);
    book.addOrder(bid2);
    book.addOrder(bid3);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), bid1);

    book.cancelOrder(1, 1);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), bid2);

    book.cancelOrder(2, 2);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), bid3);

    book.cancelOrder(3, 3);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), nullptr);
}

TEST_F(OrderBookTest, PopFrontEmptyBookNoOp) {
    EXPECT_NO_THROW(book.popFront(Side::Buy));
    EXPECT_NO_THROW(book.popFront(Side::Sell));
    EXPECT_FALSE(book.getBestBid().has_value());
    EXPECT_FALSE(book.getBestAsk().has_value());
}

TEST_F(OrderBookTest, PopFrontBuyRemovesFifoFromBestAsk) {
    OrderPtr ask1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    OrderPtr ask2 = std::make_shared<Order>(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    OrderPtr ask3 = std::make_shared<Order>(3, 3, 105, 10, Side::Sell, OrderType::Limit, 1002);

    book.addOrder(ask1);
    book.addOrder(ask2);
    book.addOrder(ask3);

    book.popFront(Side::Buy);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_TRUE(book.doesOrderExist(2));
    EXPECT_TRUE(book.doesOrderExist(3));
    EXPECT_EQ(book.getBestAsk(), std::optional<PriceTicks>{100});

    book.popFront(Side::Buy); 
    EXPECT_FALSE(book.doesOrderExist(2)); 
    EXPECT_TRUE(book.doesOrderExist(3)); 
    EXPECT_EQ(book.getBestAsk(), std::optional<PriceTicks>{105}); 
}

TEST_F(OrderBookTest, PopFrontBuyRemovesEmptyAskLevel) {
    OrderPtr ask1 = std::make_shared<Order>(1, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);
    OrderPtr ask2 = std::make_shared<Order>(2, 2, 105, 10, Side::Sell, OrderType::Limit, 1001);

    book.addOrder(ask1);
    book.addOrder(ask2);

    book.popFront(Side::Buy);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_TRUE(book.doesOrderExist(2));
    EXPECT_EQ(book.getBestAsk(), std::optional<PriceTicks>{105});

    book.popFront(Side::Buy);
    EXPECT_FALSE(book.doesOrderExist(2));
    EXPECT_EQ(book.getBestAsk(), std::nullopt);
}

TEST_F(OrderBookTest, PopFrontSellRemovesFifoFromBestBid) {
    OrderPtr bid1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = std::make_shared<Order>(2, 2, 100, 10, Side::Buy, OrderType::Limit, 1001);
    OrderPtr bid3 = std::make_shared<Order>(3, 3, 95, 10, Side::Buy, OrderType::Limit, 1002);

    book.addOrder(bid1);
    book.addOrder(bid2);
    book.addOrder(bid3);

    book.popFront(Side::Sell);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_TRUE(book.doesOrderExist(2));
    EXPECT_TRUE(book.doesOrderExist(3));
    EXPECT_EQ(book.getBestBid(), std::optional<PriceTicks>{100});

    book.popFront(Side::Sell);
    EXPECT_FALSE(book.doesOrderExist(2));
    EXPECT_TRUE(book.doesOrderExist(3));
    EXPECT_EQ(book.getBestBid(), std::optional<PriceTicks>{95});
}

TEST_F(OrderBookTest, PopFrontSellRemovesEmptyBidLevel) {
    OrderPtr bid1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = std::make_shared<Order>(2, 2, 95, 10, Side::Buy, OrderType::Limit, 1001);

    book.addOrder(bid1);
    book.addOrder(bid2);

    book.popFront(Side::Sell);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_TRUE(book.doesOrderExist(2));
    EXPECT_EQ(book.getBestBid(), std::optional<PriceTicks>{95});

    book.popFront(Side::Sell);
    EXPECT_FALSE(book.doesOrderExist(2));
    EXPECT_EQ(book.getBestBid(), std::nullopt);
}

TEST_F(OrderBookTest, TradeExecutionCountInitiallyZero) {
    EXPECT_EQ(book.getTradeExecutionCount(), 0u);
}

TEST_F(OrderBookTest, TotalVolumeTradedInitiallyZero) {
    EXPECT_EQ(book.getTotalVolumeTraded(), 0u);
}

TEST_F(OrderBookTest, RecordExecutionIgnoresNonPositiveQty) {
    book.recordExecution(-5);
    book.recordExecution(0);

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
    OrderPtr bid = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
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
}

TEST_F(OrderBookTest, SnapshotSingleAskLevelSingleAskOrder) {
    OrderPtr ask = std::make_shared<Order>(1, 1, 110, 7, Side::Sell, OrderType::Limit, 1000);
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
}

TEST_F(OrderBookTest, SnapshotSingleBidOrderSingleAskOrder) {
    OrderPtr bid = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr ask = std::make_shared<Order>(2, 2, 110, 5, Side::Sell, OrderType::Limit, 1001);
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
}

TEST_F(OrderBookTest, SnapshotSingleBidLevelMultipleBidOrders) {
    OrderPtr bid1 = std::make_shared<Order>(1, 1, 100, 3, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = std::make_shared<Order>(2, 2, 100, 7, Side::Buy, OrderType::Limit, 1001);
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
}

TEST_F(OrderBookTest, SnapshotSingleAskLevelMultipleAskOrders) {
    OrderPtr ask1 = std::make_shared<Order>(1, 1, 110, 4, Side::Sell, OrderType::Limit, 1000);
    OrderPtr ask2 = std::make_shared<Order>(2, 2, 110, 6, Side::Sell, OrderType::Limit, 1001);
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
}

TEST_F(OrderBookTest, SnapshotMultipleBidAskLevels) {
    OrderPtr bid1 = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = std::make_shared<Order>(2, 2, 105, 8, Side::Buy, OrderType::Limit, 1001);
    OrderPtr ask1 = std::make_shared<Order>(3, 3, 110, 4, Side::Sell, OrderType::Limit, 1002);
    OrderPtr ask2 = std::make_shared<Order>(4, 4, 108, 6, Side::Sell, OrderType::Limit, 1003);
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

TEST_F(OrderBookTest, SnapshotDepthLimitZeroReturnsEmptyDepthsButFullSummary) {
    OrderPtr bid1 = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = std::make_shared<Order>(2, 2, 105, 8, Side::Buy, OrderType::Limit, 1001);
    OrderPtr ask1 = std::make_shared<Order>(3, 3, 110, 4, Side::Sell, OrderType::Limit, 1002);
    book.addOrder(bid1);
    book.addOrder(bid2);
    book.addOrder(ask1);

    auto snap = book.snapshot(1234, 0);

    EXPECT_TRUE(snap.bidDepths.empty());
    EXPECT_TRUE(snap.askDepths.empty());
    EXPECT_EQ(snap.bidSummary.totalQuantity, 13);
    EXPECT_EQ(snap.bidSummary.orderCount, 2u);
    EXPECT_EQ(snap.askSummary.totalQuantity, 4);
    EXPECT_EQ(snap.askSummary.orderCount, 1u);
}

TEST_F(OrderBookTest, SnapshotDepthLimitLargerThanLevelsReturnsAll) {
    OrderPtr bid1 = std::make_shared<Order>(1, 1, 100, 5, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = std::make_shared<Order>(2, 2, 105, 8, Side::Buy, OrderType::Limit, 1001);
    OrderPtr ask1 = std::make_shared<Order>(3, 3, 110, 4, Side::Sell, OrderType::Limit, 1002);
    book.addOrder(bid1);
    book.addOrder(bid2);
    book.addOrder(ask1);

    auto snap = book.snapshot(1234, 100);

    EXPECT_EQ(snap.bidDepths.size(), 2u);
    EXPECT_EQ(snap.askDepths.size(), 1u);
}

TEST_F(OrderBookTest, SnapshotDepthLimitTwoReturnsTwoLevels) {
    OrderPtr bid1 = std::make_shared<Order>(1, 1, 95,  3, Side::Buy, OrderType::Limit, 1000);
    OrderPtr bid2 = std::make_shared<Order>(2, 2, 100, 5, Side::Buy, OrderType::Limit, 1001);
    OrderPtr bid3 = std::make_shared<Order>(3, 3, 105, 8, Side::Buy, OrderType::Limit, 1002);
    OrderPtr ask1 = std::make_shared<Order>(4, 4, 110, 4, Side::Sell, OrderType::Limit, 1003);
    OrderPtr ask2 = std::make_shared<Order>(5, 5, 115, 6, Side::Sell, OrderType::Limit, 1004);
    OrderPtr ask3 = std::make_shared<Order>(6, 6, 120, 2, Side::Sell, OrderType::Limit, 1005);
    book.addOrder(bid1);
    book.addOrder(bid2);
    book.addOrder(bid3);
    book.addOrder(ask1);
    book.addOrder(ask2);
    book.addOrder(ask3);

    auto snap = book.snapshot(1234, 2);

    ASSERT_EQ(snap.bidDepths.size(), 2u);
    ASSERT_EQ(snap.askDepths.size(), 2u);
}
