#include <gtest/gtest.h>
#include <memory>
#include "policy/modify_policy.hpp"

class MP1Test : public ::testing::Test {
protected:
    MP1 policy;

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

TEST_F(MP1Test, PriceIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP1Test, PriceDecrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(99, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP1Test, PriceUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

// =====================================================================
// Quantity-only modify
// =====================================================================

TEST_F(MP1Test, QuantityIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP1Test, QuantityDecrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP1Test, QuantityUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

// =====================================================================
// Partially filled orders
// =====================================================================

TEST_F(MP1Test, PartiallyFilled_NewTotalBelowFilled_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 3);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP1Test, PartiallyFilled_NewTotalEqualsFilled_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 4);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP1Test, PartiallyFilled_NewTotalAboveFilledButBelowOriginal_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 8);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP1Test, PartiallyFilled_NewTotalEqualsOriginal_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP1Test, PartiallyFilled_NewTotalAboveOriginal_LosesPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

// =====================================================================
// Combined price and quantity modify
// =====================================================================

TEST_F(MP1Test, PriceChangeWithQuantityIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 12);
    
    ModifyDecision decision = policy.getDecision(*resting, request);
    
    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP1Test, PriceChangeWithQuantityDecrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP1Test, PriceChangeWithQuantityUnchanged_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP1Test, SamePriceWithQuantityIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(MP1Test, SamePriceWithQuantityDecrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(MP1Test, SamePriceWithQuantityUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}
