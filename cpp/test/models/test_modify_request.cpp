#include <gtest/gtest.h>
#include "models/modify_request.hpp"

TEST(ModifyRequestTest, Validate_WellFormedPriceOnly_IsAccepted) {
    ModifyRequest request(1, 1, 1622547800, 1, 105, std::nullopt);

    RejectionReason result = request.validate();

    EXPECT_EQ(result, RejectionReason::None);
}

TEST(ModifyRequestTest, Validate_WellFormedQtyOnly_IsAccepted) {
    ModifyRequest request(1, 1, 1622547800, 1, std::nullopt, 10);

    RejectionReason result = request.validate();

    EXPECT_EQ(result, RejectionReason::None);
}

TEST(ModifyRequestTest, Validate_WellFormedPriceAndQty_IsAccepted) {
    ModifyRequest request(1, 1, 1622547800, 1, 105, 10);

    RejectionReason result = request.validate();

    EXPECT_EQ(result, RejectionReason::None);
}

TEST(ModifyRequestTest, Validate_ZeroTargetOrderID_IsRejected) {
    ModifyRequest request(1, 1, 1622547800, 0, 105, std::nullopt);

    RejectionReason result = request.validate();

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
}

TEST(ModifyRequestTest, Validate_NeitherPriceNorQtyGiven_IsRejected) {
    ModifyRequest request(1, 1, 1622547800, 1, std::nullopt, std::nullopt);

    RejectionReason result = request.validate();

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
}

TEST(ModifyRequestTest, Validate_ZeroNewPriceTicks_IsRejected) {
    ModifyRequest request(1, 1, 1622547800, 1, 0, std::nullopt);

    RejectionReason result = request.validate();

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
}

TEST(ModifyRequestTest, Validate_NegativeNewPriceTicks_IsRejected) {
    ModifyRequest request(1, 1, 1622547800, 1, -5, std::nullopt);

    RejectionReason result = request.validate();

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
}

TEST(ModifyRequestTest, Validate_ZeroNewOriginalQty_IsRejected) {
    ModifyRequest request(1, 1, 1622547800, 1, std::nullopt, 0);

    RejectionReason result = request.validate();

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
}

TEST(ModifyRequestTest, Validate_NegativeNewOriginalQty_IsRejected) {
    ModifyRequest request(1, 1, 1622547800, 1, std::nullopt, -3);

    RejectionReason result = request.validate();

    EXPECT_EQ(result, RejectionReason::InvalidModifyOrder);
}

TEST(ModifyRequestTest, Getters_ReturnConstructedValues) {
    ModifyRequest request(7, 2, 1622547801, 3, 110, 20);

    EXPECT_EQ(request.getTargetOrderID(), static_cast<OrderID>(3));
    EXPECT_EQ(request.getNewPriceTicks(), std::optional<PriceTicks>(110));
    EXPECT_EQ(request.getNewOriginalQty(), std::optional<Quantity>(20));
    EXPECT_EQ(request.getOwnerID(), static_cast<OwnerID>(2));
    EXPECT_EQ(request.getTimestamp(), static_cast<Timestamp>(1622547801));
}
