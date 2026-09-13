#pragma once
#include <cstdint>

enum class RequestType : uint8_t {
    New = 0,
    Cancel = 1,
    Modify = 2
};
