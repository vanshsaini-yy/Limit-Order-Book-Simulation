#pragma once
#include "models/cancel_request.hpp"

// The request ID is the timestamp, so it is unique within a test as long as timestamps are.
inline CancelRequest cancelRequestHelper(OrderID targetOrderID, OwnerID ownerID, Timestamp timestamp) {
    return CancelRequest(static_cast<RequestID>(timestamp), ownerID, timestamp, targetOrderID);
}
