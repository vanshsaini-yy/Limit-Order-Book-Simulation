#include<gtest/gtest.h>
#include "utils/order_utils.hpp"

TEST(IsSelfTrade, TestSelfTradeDetection) {
    auto order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto order2 = std::make_shared<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);
    auto order3 = std::make_shared<Order>(3, 2, 100, 10, Side::Sell, OrderType::Limit, 1002);

    EXPECT_TRUE(isSelfTrade(*order1, *order2));
    EXPECT_FALSE(isSelfTrade(*order1, *order3));
}