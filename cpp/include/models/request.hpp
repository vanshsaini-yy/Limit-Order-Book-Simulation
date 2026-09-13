#pragma once
#include "models/types.hpp"
#include "models/request_type.hpp"
#include "models/rejection_reason.hpp"

class IRequest {
private:
    RequestID requestId;
    RequestType requestType;
    OwnerID ownerID;
    Timestamp timestamp;

public:
    IRequest(RequestID requestId_, RequestType requestType_, OwnerID ownerID_, Timestamp timestamp_);
    virtual ~IRequest() = default;

    RequestID   getRequestId()   const;
    RequestType getRequestType() const;
    OwnerID     getOwnerID()     const;
    Timestamp   getTimestamp()   const;

    virtual RejectionReason validate() const = 0;
};
