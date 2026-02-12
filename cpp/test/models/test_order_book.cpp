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

TEST_F(OrderBookTest, RemoveOrderSuccess) {
    OrderPtr order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(order1);
    book.addOrder(order2);

    EXPECT_EQ(book.removeOrder(1), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.removeOrder(2), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(2));

    delete order1;
    delete order2;
}

TEST_F(OrderBookTest, RemoveNonExistingOrderFails) {
    EXPECT_EQ(book.removeOrder(999), RejectionReason::OrderToBeRemovedDoesNotExist);
    EXPECT_FALSE(book.doesOrderExist(999));
    EXPECT_EQ(book.removeOrder(1001), RejectionReason::OrderToBeRemovedDoesNotExist);
    EXPECT_FALSE(book.doesOrderExist(1001));
}

TEST_F(OrderBookTest, RemoveCancelledOrderFails) {
    OrderPtr order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    OrderPtr order2 = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);
    book.addOrder(order1);
    book.addOrder(order2);
    order1->setStatus(OrderStatus::Cancelled);
    order2->setStatus(OrderStatus::CancelledAfterPartialExecution);

    EXPECT_EQ(book.removeOrder(1), RejectionReason::OrderBookInvariantViolation);
    EXPECT_EQ(book.removeOrder(2), RejectionReason::OrderBookInvariantViolation);

    delete order1;
    delete order2;
}

TEST_F(OrderBookTest, RemoveExecutedOrderFails) {
    OrderPtr order = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);
    order->setStatus(OrderStatus::Executed);

    EXPECT_EQ(book.removeOrder(1), RejectionReason::OrderBookInvariantViolation);

    delete order;
}

TEST_F(OrderBookTest, DoubleRemoveFails) {
    OrderPtr order = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);

    EXPECT_EQ(book.removeOrder(1), RejectionReason::None);
    EXPECT_FALSE(book.doesOrderExist(1));
    EXPECT_EQ(book.removeOrder(1), RejectionReason::OrderToBeRemovedDoesNotExist);
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

    book.removeOrder(2);
    book.removeOrder(3);

    EXPECT_EQ(book.getBestBid(), 100ull);
    EXPECT_EQ(book.getBestAsk(), 115ull);

    book.removeOrder(1);
    book.removeOrder(4);

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

    book.removeOrder(1);
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), ask2);

    book.removeOrder(2);
    EXPECT_EQ(book.getMatchedOrder(Side::Buy), ask3);

    book.removeOrder(3); 
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

    book.removeOrder(1);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), bid2);

    book.removeOrder(2);
    EXPECT_EQ(book.getMatchedOrder(Side::Sell), bid3);

    book.removeOrder(3);
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