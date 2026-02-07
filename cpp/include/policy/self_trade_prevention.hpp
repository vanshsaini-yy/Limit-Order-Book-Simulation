#pragma once
#include<cstdint>
#include "models/order.hpp"

enum class STPProtocol : uint8_t {
    CancelNewest = 0,
    CancelOldest = 1,
    CancelBoth = 2,
};

namespace stp {
    bool isSelfTrade(const OrderPtr &order1, const OrderPtr &order2) {
        return order1->getOwnerID() == order2->getOwnerID();
    }
    
    OrderPtr getOlderOrder(const OrderPtr &order1, const OrderPtr &order2) {
        uint64_t timestamp1 = order1->getTimestamp();
        uint64_t timestamp2 = order2->getTimestamp();
        if (timestamp1 != timestamp2) {
            return (timestamp1 < timestamp2) ? order1 : order2;
        }
        return (order1->getOrderID() < order2->getOrderID()) ? order1 : order2;
    }
}