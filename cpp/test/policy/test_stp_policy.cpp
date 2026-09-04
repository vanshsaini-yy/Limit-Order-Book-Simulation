#include <gtest/gtest.h>
#include "policy/stp_policy.hpp"

TEST(STPPolicyTest, CancelBothSTP_CancelsIncomingAndResting) {
    CancelBothSTP policy;
    STPDecision decision = policy.getDecision();
    EXPECT_TRUE(decision.cancelIncoming);
    EXPECT_TRUE(decision.cancelResting);
}

TEST(STPPolicyTest, CancelIncomingSTP_CancelsOnlyIncoming) {
    CancelIncomingSTP policy;
    STPDecision decision = policy.getDecision();
    EXPECT_TRUE(decision.cancelIncoming);
    EXPECT_FALSE(decision.cancelResting);
}

TEST(STPPolicyTest, CancelRestingSTP_CancelsOnlyResting) {
    CancelRestingSTP policy;
    STPDecision decision = policy.getDecision();
    EXPECT_FALSE(decision.cancelIncoming);
    EXPECT_TRUE(decision.cancelResting);
}