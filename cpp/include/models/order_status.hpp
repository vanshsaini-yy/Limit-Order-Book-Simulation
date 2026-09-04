#pragma once
#include <cstdint>

enum class OrderStatus : uint16_t {
    Pending = 0,
    PartiallyExecuted = 1,
    Executed = 2,
    Cancelled = 3,
    CancelledAfterPartialExecution = 4
};