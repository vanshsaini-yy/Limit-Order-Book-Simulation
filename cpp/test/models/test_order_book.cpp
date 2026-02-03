#include <gtest/gtest.h>
#include <memory>
#include "models/order.hpp"
#include "models/order_index.hpp"
#include "models/order_book.hpp"

class OrderBookTest : public ::testing::Test {
protected:
    LimitOrderBook book;
};

TEST_F(OrderBookTest, DefaultBehaviour) {
    EXPECT_EQ(book.getSTPProtocol(), STPProtocol::CancelBoth);
}

TEST_F(OrderBookTest, CustomSTPProtocol) {
    LimitOrderBook stpBook(STPProtocol::CancelNewest);
    EXPECT_EQ(stpBook.getSTPProtocol(), STPProtocol::CancelNewest);
}

TEST_F(OrderBookTest, AddOrderSuccess) {
    auto order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto order2 = new Order(2, 2, 105, 10, Side::Sell, OrderType::Limit, 1001);
    EXPECT_TRUE(book.addOrder(order1));
    EXPECT_TRUE(book.addOrder(order2));
}

TEST_F(OrderBookTest, AddZeroQtyOrderFails) {
    auto zeroQtyOrder = new Order(1, 1, 100, 0, Side::Buy, OrderType::Limit, 1000);
    EXPECT_FALSE(book.addOrder(zeroQtyOrder));
}

TEST_F(OrderBookTest, AddMarketOrderFails) {
    auto marketOrder = new Order(1, 1, 0, 10, Side::Buy, OrderType::Market, 1000);
    EXPECT_FALSE(book.addOrder(marketOrder));
}

TEST_F(OrderBookTest, CancelOrderSuccess) {
    auto order = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);
    EXPECT_TRUE(book.cancelOrder(1));
}

TEST_F(OrderBookTest, CancelNonExistingOrderFails) {
    EXPECT_FALSE(book.cancelOrder(999));
}

TEST_F(OrderBookTest, BidAskIndependent) {
    auto buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto sellOrder = new Order(2, 2, 100, 10, Side::Sell, OrderType::Limit, 1001);

    book.addOrder(buyOrder);
    book.addOrder(sellOrder);

    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_TRUE(book.cancelOrder(2));
}

TEST_F(OrderBookTest, DoubleCancel) {
    auto order = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    book.addOrder(order);

    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_FALSE(book.cancelOrder(1));
}

TEST_F(OrderBookTest, GetBestBidAsk) {
    auto buyOrder1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto buyOrder2 = new Order(2, 2, 105, 10, Side::Buy, OrderType::Limit, 1001);
    auto sellOrder1 = new Order(3, 3, 110, 10, Side::Sell, OrderType::Limit, 1002);
    auto sellOrder2 = new Order(4, 4, 115, 10, Side::Sell, OrderType::Limit, 1003);

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

    EXPECT_EQ(book.getBestBid(), 0ull);
    EXPECT_EQ(book.getBestAsk(), 0ull);
}

TEST_F(OrderBookTest, IsOrderMarketableLimitOrders) {
    auto buyOrder = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto sellOrder = new Order(2, 2, 110, 10, Side::Sell, OrderType::Limit, 1001);

    book.addOrder(buyOrder);
    book.addOrder(sellOrder);

    auto marketableBuy = new Order(3, 3, 115, 10, Side::Buy, OrderType::Limit, 1002);
    auto nonMarketableBuy = new Order(4, 4, 90, 10, Side::Buy, OrderType::Limit, 1003);
    auto marketableSell = new Order(5, 5, 95, 10, Side::Sell, OrderType::Limit, 1004);
    auto nonMarketableSell = new Order(6, 6, 120, 10, Side::Sell, OrderType::Limit, 1005);

    EXPECT_TRUE(book.isOrderMarketable(marketableBuy));
    EXPECT_FALSE(book.isOrderMarketable(nonMarketableBuy));
    EXPECT_TRUE(book.isOrderMarketable(marketableSell));
    EXPECT_FALSE(book.isOrderMarketable(nonMarketableSell));
}

TEST_F(OrderBookTest, IsOrderMarketableMarketOrders) {
    auto marketBuyOrder = new Order(1, 1, 0, 10, Side::Buy, OrderType::Market, 1000);
    auto marketSellOrder = new Order(2, 2, 0, 10, Side::Sell, OrderType::Market, 1001);

    EXPECT_TRUE(book.isOrderMarketable(marketBuyOrder));
    EXPECT_TRUE(book.isOrderMarketable(marketSellOrder));
}

TEST_F(OrderBookTest, IsOrderMarketableZeroQtyOrder) {
    auto zeroQtyLimitOrder = new Order(1, 1, 100, 0, Side::Buy, OrderType::Limit, 1000);
    auto zeroQtyMarketOrder = new Order(2, 2, 0, 0, Side::Sell, OrderType::Market, 1001);

    EXPECT_FALSE(book.isOrderMarketable(zeroQtyLimitOrder));
    EXPECT_FALSE(book.isOrderMarketable(zeroQtyMarketOrder));
}

TEST_F(OrderBookTest, IsSelfTrade) {
    auto order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto order2 = new Order(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);
    auto order3 = new Order(3, 2, 100, 10, Side::Sell, OrderType::Limit, 1002);
    EXPECT_TRUE(book.isSelfTrade(order1, order2));
    EXPECT_FALSE(book.isSelfTrade(order1, order3));
}

TEST_F(OrderBookTest, EnforceSTPCancelBoth) {
    LimitOrderBook stpBook(STPProtocol::CancelBoth);
    auto order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto order2 = new Order(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);

    stpBook.addOrder(order1);
    stpBook.addOrder(order2);

    stpBook.enforceSTP(order1, order2);

    EXPECT_FALSE(stpBook.cancelOrder(1));
    EXPECT_FALSE(stpBook.cancelOrder(2));
}

TEST_F(OrderBookTest, EnforceSTPCancelNewest) {
    LimitOrderBook stpBook(STPProtocol::CancelNewest);
    auto order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto order2 = new Order(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);

    stpBook.addOrder(order1);
    stpBook.addOrder(order2);

    stpBook.enforceSTP(order1, order2);

    EXPECT_TRUE(stpBook.cancelOrder(1));
    EXPECT_FALSE(stpBook.cancelOrder(2));
}

TEST_F(OrderBookTest, EnforceSTPCancelOldest) {
    LimitOrderBook stpBook(STPProtocol::CancelOldest);
    auto order1 = new Order(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto order2 = new Order(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);

    stpBook.addOrder(order1);
    stpBook.addOrder(order2);

    stpBook.enforceSTP(order1, order2);

    EXPECT_FALSE(stpBook.cancelOrder(1));
    EXPECT_TRUE(stpBook.cancelOrder(2));
}