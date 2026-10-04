#include <gtest/gtest.h>
#include <memory>
#include "policy/modify_policy.hpp"

class QuantityKeepsPriorityModifyPolicyTest : public ::testing::Test {
protected:
    QuantityKeepsPriorityModifyPolicy policy;

    OrderPtr makeResting(Quantity originalQty, Quantity filledQty) {
        OrderPtr order = std::make_shared<Order>(1, 1, 100, originalQty, Side::Buy, OrderType::Limit, 1000);
        order->reduceQty(filledQty);
        return order;
    }

    ModifyRequest makeRequest(std::optional<PriceTicks> newPriceTicks, std::optional<Quantity> newOriginalQty) {
        return ModifyRequest(1, 1, 1001, 1, newPriceTicks, newOriginalQty);
    }
};

TEST_F(QuantityKeepsPriorityModifyPolicyTest, PriceIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(QuantityKeepsPriorityModifyPolicyTest, PriceDecrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(99, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}

TEST_F(QuantityKeepsPriorityModifyPolicyTest, PriceUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, std::nullopt);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(QuantityKeepsPriorityModifyPolicyTest, QuantityIncrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(QuantityKeepsPriorityModifyPolicyTest, QuantityDecrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 6);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(QuantityKeepsPriorityModifyPolicyTest, QuantityUnchanged_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(std::nullopt, 10);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(QuantityKeepsPriorityModifyPolicyTest, PartiallyFilled_NewTotalAboveOriginal_KeepsPriority) {
    OrderPtr resting = makeResting(10, 4);
    ModifyRequest request = makeRequest(std::nullopt, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(QuantityKeepsPriorityModifyPolicyTest, SamePriceWithQuantityIncrease_KeepsPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(100, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_FALSE(decision.losesPriority);
}

TEST_F(QuantityKeepsPriorityModifyPolicyTest, PriceChangeWithQuantityIncrease_LosesPriority) {
    OrderPtr resting = makeResting(10, 0);
    ModifyRequest request = makeRequest(101, 12);

    ModifyDecision decision = policy.getDecision(*resting, request);

    EXPECT_TRUE(decision.losesPriority);
}
