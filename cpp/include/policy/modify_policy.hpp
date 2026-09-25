#pragma once
#include "models/order.hpp"
#include "models/modify_request.hpp"

struct ModifyDecision {
    bool losesPriority;
};

class ModifyPolicy {
public:
    virtual ~ModifyPolicy() = default;
    virtual ModifyDecision getDecision(const Order &resting, const ModifyRequest &request) const = 0;
};

class DefaultModifyPolicy final : public ModifyPolicy {
public:
    ModifyDecision getDecision(const Order &resting, const ModifyRequest &request) const override;
};
