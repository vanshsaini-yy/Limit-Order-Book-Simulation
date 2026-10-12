#include <gtest/gtest.h>
#include <memory>
#include "policy/modify_policy.hpp"

class MP2Test : public ::testing::Test {
protected:
    MP2 policy;

    OrderPtr makeResting(Quantity originalQty, Quantity filledQty) {
        OrderPtr order = std::make_shared<Order>(1, 1, 100, originalQty, Side::Buy, OrderType::Limit, 1000);
        order->reduceQty(filledQty);
        return order;
    }

    ModifyRequest makeRequest(std::optional<PriceTicks> newPriceTicks, std::optional<Quantity> newOriginalQty) {
        return ModifyRequest(1, 1, 1001, 1, newPriceTicks, newOriginalQty);
    }
};

// =====================================================================
// Price-only modify
// =====================================================================

TEST_F(MP2Test, PriceIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP2Test, PriceDecrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(99, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP2Test, PriceUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

// =====================================================================
// Quantity-only modify
// =====================================================================

TEST_F(MP2Test, QuantityIncrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP2Test, QuantityDecrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP2Test, QuantityUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

// =====================================================================
// Partially filled orders
// =====================================================================

TEST_F(MP2Test, PartiallyFilled_NewTotalBelowFilled_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 3);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP2Test, PartiallyFilled_NewTotalEqualsFilled_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 4);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP2Test, PartiallyFilled_NewTotalAboveFilledButBelowOriginal_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 8);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP2Test, PartiallyFilled_NewTotalEqualsOriginal_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP2Test, PartiallyFilled_NewTotalAboveOriginal_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

// =====================================================================
// Combined price and quantity modify
// =====================================================================

TEST_F(MP2Test, PriceChangeWithQuantityIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP2Test, PriceChangeWithQuantityDecrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP2Test, PriceChangeWithQuantityUnchanged_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP2Test, SamePriceWithQuantityIncrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP2Test, SamePriceWithQuantityDecrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP2Test, SamePriceWithQuantityUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}
