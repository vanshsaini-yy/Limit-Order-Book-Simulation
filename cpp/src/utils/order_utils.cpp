#include "utils/order_utils.hpp"

bool isSelfTrade(const Order& order1, const Order& order2) {
    return order1.getOwnerID() == order2.getOwnerID();
}
