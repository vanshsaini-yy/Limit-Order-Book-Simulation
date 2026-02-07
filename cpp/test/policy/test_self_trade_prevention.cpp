#include<gtest/gtest.h>
#include "policy/self_trade_prevention.hpp"

class SelfTradePreventionTest : public ::testing::Test {
};

TEST_F(SelfTradePreventionTest, IsSelfTrade) {
    auto order1 = std::make_unique<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto order2 = std::make_unique<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);
    auto order3 = std::make_unique<Order>(3, 2, 100, 10, Side::Sell, OrderType::Limit, 1002);

    EXPECT_TRUE(stp::isSelfTrade(order1.get(), order2.get()));
    EXPECT_FALSE(stp::isSelfTrade(order1.get(), order3.get()));
}

TEST_F(SelfTradePreventionTest, GetOlderOrder) {
    auto order1 = std::make_unique<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1000);
    auto order2 = std::make_unique<Order>(2, 1, 100, 10, Side::Sell, OrderType::Limit, 1001);
    auto order3 = std::make_unique<Order>(3, 1, 100, 10, Side::Sell, OrderType::Limit, 1000);

    EXPECT_EQ(stp::getOlderOrder(order1.get(), order2.get()), order1.get());
    EXPECT_EQ(stp::getOlderOrder(order1.get(), order3.get()), order1.get());
    EXPECT_EQ(stp::getOlderOrder(order2.get(), order3.get()), order3.get());
}