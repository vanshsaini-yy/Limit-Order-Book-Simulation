#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.hpp"
#include "models/cancel_request.hpp"

class MatchingEngineCancelTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook orderBook;
    MatchingEngine engine{&orderBook, &stpPolicy};
};

TEST_F(MatchingEngineCancelTest, Submit_CancelsRestingOrder) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    engine.matchOrder(order);

    CancelRequest request(1, 1, 1622547801, order->getOrderID());
    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::None);
    EXPECT_EQ(result.order, order);
    EXPECT_EQ(order->getStatus(), OrderStatus::Cancelled);
    EXPECT_FALSE(orderBook.doesOrderExist(order->getOrderID()));
    EXPECT_EQ(orderBook.getOrderCancellationCount(), 1u);
}

TEST_F(MatchingEngineCancelTest, Submit_NonExistentOrder_IsRejected) {
    OrderID nonExistentOrderID = 999;
    CancelRequest request(1, 1, 1622547800, nonExistentOrderID);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), 0u);
}

TEST_F(MatchingEngineCancelTest, Submit_AnotherOwnersOrder_IsRejected) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    engine.matchOrder(order);

    CancelRequest request(1, 2, 1622547801, order->getOrderID());
    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::OrderToBeCancelledDoesNotExist);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(order->getStatus(), OrderStatus::Pending);
    EXPECT_TRUE(orderBook.doesOrderExist(order->getOrderID()));
    EXPECT_EQ(orderBook.getOrderCancellationCount(), 0u);
}

TEST_F(MatchingEngineCancelTest, Submit_AnotherOwnersOrder_IsIndistinguishableFromNonExistentOrder) {
    OrderPtr order = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    engine.matchOrder(order);

    OwnerID anotherOwnerID = 2;
    OrderID nonExistentOrderID = 999;
    SubmitResult wrongOwner = engine.submit(CancelRequest(1, anotherOwnerID, 1622547801, order->getOrderID()));
    SubmitResult nonExistent = engine.submit(CancelRequest(2, anotherOwnerID, 1622547802, nonExistentOrderID));

    EXPECT_EQ(wrongOwner.reason, nonExistent.reason);
    EXPECT_EQ(wrongOwner.order, nonExistent.order);
    EXPECT_EQ(wrongOwner.order, nullptr);
}

TEST_F(MatchingEngineCancelTest, Submit_PartiallyFilledOrder_LandsInCancelledAfterPartialExecution) {
    OrderPtr order1 = std::make_shared<Order>(1, 1, 100, 10, Side::Buy, OrderType::Limit, 1622547800);
    OrderPtr order2 = std::make_shared<Order>(2, 2, 100, 5, Side::Sell, OrderType::Limit, 1622547801);
    engine.matchOrder(order1);
    engine.matchOrder(order2);

    CancelRequest request(1, 1, 1622547802, order1->getOrderID());
    EXPECT_EQ(engine.submit(request).reason, RejectionReason::None);

    EXPECT_EQ(order1->getStatus(), OrderStatus::CancelledAfterPartialExecution);
    EXPECT_FALSE(orderBook.doesOrderExist(order1->getOrderID()));
    EXPECT_EQ(orderBook.getOrderCancellationCount(), 1u);
}

TEST_F(MatchingEngineCancelTest, Submit_InvalidRequest_IsRejected_AndDoesNotIncrementCount) {
    CancelRequest request(1, 1, 1622547800, 0);

    SubmitResult result = engine.submit(request);

    EXPECT_EQ(result.reason, RejectionReason::InvalidCancelOrder);
    EXPECT_EQ(result.order, nullptr);
    EXPECT_EQ(orderBook.getOrderCancellationCount(), 0u);
}
