#pragma once
#include <cstdint>

enum class TimeInForce : uint8_t {
    GTC = 0,
    IOC = 1,
    FOK = 2
};