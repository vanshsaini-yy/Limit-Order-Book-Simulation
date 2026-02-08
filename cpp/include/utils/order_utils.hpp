#include "models/order.hpp"

bool isSelfTrade(const OrderPtr &order1, const OrderPtr &order2) {
    return order1->getOwnerID() == order2->getOwnerID();
}