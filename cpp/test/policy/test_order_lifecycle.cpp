#include <gtest/gtest.h>
#include "policy/order_lifecycle.hpp"

TEST(OrderLifecycleTest, AfterCancelIncoming_UntouchedOrder_IsCancelled) {
    EXPECT_EQ(OrderLifecycle::afterCancelIncoming(10, 10), OrderStatus::Cancelled);
}

TEST(OrderLifecycleTest, AfterCancelIncoming_PartiallyFilledOrder_IsCancelledAfterPartialExecution) {
    EXPECT_EQ(OrderLifecycle::afterCancelIncoming(10, 5), OrderStatus::CancelledAfterPartialExecution);
}

TEST(OrderLifecycleTest, AfterCancelResting_Pending_BecomesCancelled) {
    EXPECT_EQ(OrderLifecycle::afterCancelResting(OrderStatus::Pending), OrderStatus::Cancelled);
}

TEST(OrderLifecycleTest, AfterCancelResting_PartiallyExecuted_BecomesCancelledAfterPartialExecution) {
    EXPECT_EQ(OrderLifecycle::afterCancelResting(OrderStatus::PartiallyExecuted), OrderStatus::CancelledAfterPartialExecution);
}

TEST(OrderLifecycleTest, AfterMatching_CancelType_AlwaysExecuted) {
    EXPECT_EQ(OrderLifecycle::afterMatching(0, 0, OrderType::Cancel), OrderStatus::Executed);
}

TEST(OrderLifecycleTest, AfterMatching_FullyFilled_LimitIsExecuted) {
    EXPECT_EQ(OrderLifecycle::afterMatching(10, 0, OrderType::Limit), OrderStatus::Executed);
}

TEST(OrderLifecycleTest, AfterMatching_FullyFilled_MarketIsExecuted) {
    EXPECT_EQ(OrderLifecycle::afterMatching(10, 0, OrderType::Market), OrderStatus::Executed);
}

TEST(OrderLifecycleTest, AfterMatching_PartiallyFilled_LimitIsPartiallyExecuted) {
    EXPECT_EQ(OrderLifecycle::afterMatching(10, 5, OrderType::Limit), OrderStatus::PartiallyExecuted);
}

TEST(OrderLifecycleTest, AfterMatching_PartiallyFilled_MarketIsCancelledAfterPartialExecution) {
    EXPECT_EQ(OrderLifecycle::afterMatching(10, 5, OrderType::Market), OrderStatus::CancelledAfterPartialExecution);
}

TEST(OrderLifecycleTest, AfterMatching_Untouched_LimitIsPending) {
    EXPECT_EQ(OrderLifecycle::afterMatching(10, 10, OrderType::Limit), OrderStatus::Pending);
}

TEST(OrderLifecycleTest, AfterMatching_Untouched_MarketIsCancelled) {
    EXPECT_EQ(OrderLifecycle::afterMatching(10, 10, OrderType::Market), OrderStatus::Cancelled);
}
