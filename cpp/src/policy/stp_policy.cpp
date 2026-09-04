#include "policy/stp_policy.hpp"

STPDecision CancelBothSTP::getDecision() const {
    return STPDecision{true, true};
}

STPDecision CancelIncomingSTP::getDecision() const {
    return STPDecision{true, false};
}

STPDecision CancelRestingSTP::getDecision() const {
    return STPDecision{false, true};
}
