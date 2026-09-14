#include <gtest/gtest.h>
#include "models/cancel_request.hpp"

TEST(CancelRequestTest, Validate_WellFormed_IsAccepted) {
    CancelRequest request(1, 1, 1622547800, 1);

    EXPECT_EQ(request.validate(), RejectionReason::None);
}

TEST(CancelRequestTest, Validate_ZeroTargetOrderID_IsRejected) {
    CancelRequest request(1, 1, 1622547800, 0);

    EXPECT_EQ(request.validate(), RejectionReason::InvalidCancelOrder);
}
