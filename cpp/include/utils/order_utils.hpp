#pragma once
#include "models/order.hpp"

bool isSelfTrade(const OrderPtr &order1, const OrderPtr &order2);
