#pragma once
#include <cstdint>
#include "models/order.hpp"

struct STPDecision {
    bool cancelIncoming = false;
    bool cancelResting = false;
};

class STPPolicy {
public:
    virtual ~STPPolicy() = default;
    virtual STPDecision getDecision() const = 0;
};

class CancelBothSTP final : public STPPolicy {
public:
    STPDecision getDecision() const override;
};

class CancelIncomingSTP final : public STPPolicy {
public:
    STPDecision getDecision() const override;
};

class CancelRestingSTP final : public STPPolicy {
public:
    STPDecision getDecision() const override;
};
