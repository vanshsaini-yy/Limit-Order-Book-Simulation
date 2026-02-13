#include <gtest/gtest.h>
#include "models/trade.hpp"

class TradeTest : public ::testing::Test {
protected:
    Trade* trade;

    void SetUp() override {
        trade = new Trade(1001, 11, 22, 12345, 50, Side::Buy, 1622547800);
    }

    void TearDown() override {
        delete trade;
    }
};

TEST_F(TradeTest, GettersReturnExpectedValues) {
    EXPECT_EQ(trade->getTradeID(), 1001);
    EXPECT_EQ(trade->getTimestamp(), 1622547800);
    EXPECT_EQ(trade->getPriceTicks(), 12345);
    EXPECT_EQ(trade->getQty(), 50);
    EXPECT_EQ(trade->getSide(), Side::Buy);
    EXPECT_EQ(trade->getTakerOrderID(), 11);
    EXPECT_EQ(trade->getMakerOrderID(), 22);
}
