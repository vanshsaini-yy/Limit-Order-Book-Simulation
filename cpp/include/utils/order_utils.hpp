#pragma once
#include "models/order.hpp"

bool isSelfTrade(const Order& order1, const Order& order2);
