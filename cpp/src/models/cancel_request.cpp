#include "models/cancel_request.hpp"

CancelRequest::CancelRequest(RequestID requestId_, OwnerID ownerID_, Timestamp timestamp_, OrderID targetOrderID_)
:   IRequest(requestId_, RequestType::Cancel, ownerID_, timestamp_),
    targetOrderID(targetOrderID_) {}

OrderID CancelRequest::getTargetOrderID() const { return targetOrderID; }

RejectionReason CancelRequest::validate() const {
    if (targetOrderID == 0) {
        return RejectionReason::InvalidCancelOrder;
    }
    return RejectionReason::None;
}
