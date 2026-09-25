#include <gtest/gtest.h>
#include <memory>
#include "policy/modify_policy.hpp"

class DefaultModifyPolicyTest : public ::testing::Test {
protected:
    DefaultModifyPolicy policy;

    OrderPtr makeResting(Quantity originalQty, Quantity filledQty) {
        OrderPtr order = std::make_shared<Order>(1, 1, 100, originalQty, Side::Buy, OrderType::Limit, 1000);
        order->reduceQty(filledQty);
        return order;
    }

    ModifyRequest makeRequest(std::optional<PriceTicks> newPriceTicks, std::optional<Quantity> newOriginalQty) {
        return ModifyRequest(1, 1, 2000, 1, newPriceTicks, newOriginalQty);
    }
};

TEST_F(DefaultModifyPolicyTest, PriceIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, PriceDecrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(99, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, PriceUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, QuantityIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, QuantityDecrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, QuantityUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, PartiallyFilled_NewTotalAboveRemainingButBelowOriginal_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 8);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, PartiallyFilled_NewTotalEqualsOriginal_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, PartiallyFilled_NewTotalAboveOriginal_LosesPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, PriceChangeWithQuantityIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 12);
    
    ModifyDecision decision = policy.getDecision(*resting, request);
    
    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, PriceChangeWithQuantityDecrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, PriceChangeWithQuantityUnchanged_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, SamePriceWithQuantityIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, SamePriceWithQuantityDecrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(DefaultModifyPolicyTest, SamePriceWithQuantityUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}
