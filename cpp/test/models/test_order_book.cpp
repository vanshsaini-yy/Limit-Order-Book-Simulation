#include <gtest/gtest.h>
#include <memory>
#include "models/order.hpp"
#include "models/order_index.hpp"
#include "models/order_book.hpp"

class OrderBookTest : public ::testing::Test {
protected:
    LimitOrderBook book;
};

TEST_F(OrderBookTest, CancelExistingOrder) {
    auto order = std::make_shared<Order>(
        1, 100, 10, Side::Buy, OrderType::Limit, 123
    );

    book.addOrder(order);
    EXPECT_TRUE(book.cancelOrder(1));
}

TEST_F(OrderBookTest, CancelNonExistingOrder) {
    EXPECT_FALSE(book.cancelOrder(999));
}

TEST_F(OrderBookTest, FIFOAtSamePrice) {
    auto o1 = std::make_shared<Order>(1, 100, 10, Side::Buy, OrderType::Limit, 1);
    auto o2 = std::make_shared<Order>(2, 100, 10, Side::Buy, OrderType::Limit, 2);
    auto o3 = std::make_shared<Order>(3, 100, 10, Side::Buy, OrderType::Limit, 3);

    book.addOrder(o1);
    book.addOrder(o2);
    book.addOrder(o3);

    EXPECT_TRUE(book.cancelOrder(2));
    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_TRUE(book.cancelOrder(3));
}

TEST_F(OrderBookTest, BidAskIndependent) {
    auto buy = std::make_shared<Order>(1, 100, 10, Side::Buy, OrderType::Limit, 1);
    auto sell = std::make_shared<Order>(2, 100, 10, Side::Sell, OrderType::Limit, 2);

    book.addOrder(buy);
    book.addOrder(sell);

    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_TRUE(book.cancelOrder(2));
}

TEST_F(OrderBookTest, MultiplePrices) {
    auto o1 = std::make_shared<Order>(1, 100, 10, Side::Buy, OrderType::Limit, 1);
    auto o2 = std::make_shared<Order>(2, 101, 10, Side::Buy, OrderType::Limit, 2);
    auto o3 = std::make_shared<Order>(3, 99,  10, Side::Buy, OrderType::Limit, 3);

    book.addOrder(o1);
    book.addOrder(o2);
    book.addOrder(o3);

    EXPECT_TRUE(book.cancelOrder(2));
    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_TRUE(book.cancelOrder(3));
}

TEST_F(OrderBookTest, DoubleCancel) {
    auto o1 = std::make_shared<Order>(1, 100, 10, Side::Buy, OrderType::Limit, 1);
    book.addOrder(o1);

    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_FALSE(book.cancelOrder(1));
}
