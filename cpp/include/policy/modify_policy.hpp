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

// MP1: a price change or a quantity increase loses queue priority; a quantity decrease keeps it.
class MP1 final : public ModifyPolicy {
public:
    ModifyDecision getDecision(const Order &resting, const ModifyRequest &request) const override;
};

// MP2: only a price change loses queue priority; any quantity change keeps it.
class MP2 final : public ModifyPolicy {
public:
    ModifyDecision getDecision(const Order &resting, const ModifyRequest &request) const override;
};
