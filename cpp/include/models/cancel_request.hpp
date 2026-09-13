#pragma once
#include "models/request.hpp"

class CancelRequest : public IRequest {
private:
    OrderID targetOrderID;

public:
    CancelRequest(RequestID requestId_, OwnerID ownerID_, Timestamp timestamp_, OrderID targetOrderID_);

    OrderID getTargetOrderID() const;

    RejectionReason validate() const override;
};
