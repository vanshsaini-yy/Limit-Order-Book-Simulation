#pragma once
#include<cstdint>
#include "models/order.hpp"
#include "models/order_lifecycle.hpp"

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
        STPDecision getDecision() const override {
            STPDecision decision;
            decision.cancelIncoming = true;
            decision.cancelResting = true;
            return decision;
        }
};

class CancelIncomingSTP final : public STPPolicy {
    public:
        STPDecision getDecision() const override {
            STPDecision decision;
            decision.cancelIncoming = true;
            decision.cancelResting = false;
            return decision;
        }
};

class CancelRestingSTP final : public STPPolicy {
    public:
        STPDecision getDecision() const override {
            STPDecision decision;
            decision.cancelIncoming = false;
            decision.cancelResting = true;
            return decision;
        }
};