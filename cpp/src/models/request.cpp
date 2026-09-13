#include "models/request.hpp"

IRequest::IRequest(RequestID requestId_, RequestType requestType_, OwnerID ownerID_, Timestamp timestamp_)
:   requestId(requestId_),
    requestType(requestType_),
    ownerID(ownerID_),
    timestamp(timestamp_) {}

RequestID   IRequest::getRequestId()   const { return requestId; }
RequestType IRequest::getRequestType() const { return requestType; }
OwnerID     IRequest::getOwnerID()     const { return ownerID; }
Timestamp   IRequest::getTimestamp()   const { return timestamp; }
