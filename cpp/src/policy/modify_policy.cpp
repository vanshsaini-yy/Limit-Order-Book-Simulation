#include "policy/modify_policy.hpp"

ModifyDecision DefaultModifyPolicy::getDecision(const Order &resting, const ModifyRequest &request) const {
    std::optional<PriceTicks> newPriceTicks = request.getNewPriceTicks();
    if (newPriceTicks.has_value() && *newPriceTicks != resting.getPriceTicks()) {
        return ModifyDecision{true};
    }

    std::optional<Quantity> newOriginalQty = request.getNewOriginalQty();
    if (newOriginalQty.has_value() && *newOriginalQty > resting.getOriginalQty()) {
        return ModifyDecision{true};
    }

    return ModifyDecision{false};
}

ModifyDecision QuantityKeepsPriorityModifyPolicy::getDecision(const Order &resting, const ModifyRequest &request) const {
    std::optional<PriceTicks> newPriceTicks = request.getNewPriceTicks();
    if (newPriceTicks.has_value() && *newPriceTicks != resting.getPriceTicks()) {
        return ModifyDecision{true};
    }

    return ModifyDecision{false};
}
