#pragma once
#include <cstdint>

enum class OrderType : uint8_t {
    Limit = 0,
    Market = 1,
    Cancel = 2
};