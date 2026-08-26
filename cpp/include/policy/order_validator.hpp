#pragma once
#include "models/order.hpp"
#include "models/rejection_reason.hpp"

class OrderValidator {
    public:
        static RejectionReason validateLimitOrder(const OrderPtr &order, bool allowPartialExecution = false);
        static RejectionReason validateMarketOrder(const OrderPtr &order);
        static RejectionReason validateCancelOrder(const OrderPtr &order);
        static RejectionReason validateBeforeAddingOrRemoving(const OrderPtr &order);
        static RejectionReason validateBeforeMatching(const OrderPtr &order);
};
