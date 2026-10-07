#pragma once
#include "models/order.hpp"
#include "models/rejection_reason.hpp"

struct SubmitResult {
    RejectionReason reason;
    OrderPtr order;
};
