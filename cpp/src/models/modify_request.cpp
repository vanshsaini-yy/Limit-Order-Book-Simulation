#include "models/modify_request.hpp"

ModifyRequest::ModifyRequest(
    RequestID requestId_,
    OwnerID ownerID_,
    Timestamp timestamp_,
    OrderID targetOrderID_,
    std::optional<PriceTicks> newPriceTicks_,
    std::optional<Quantity> newOriginalQty_
)
:   IRequest(requestId_, RequestType::Modify, ownerID_, timestamp_),
    targetOrderID(targetOrderID_),
    newPriceTicks(newPriceTicks_),
    newOriginalQty(newOriginalQty_) {}

OrderID                   ModifyRequest::getTargetOrderID()    const { return targetOrderID; }
std::optional<PriceTicks> ModifyRequest::getNewPriceTicks()    const { return newPriceTicks; }
std::optional<Quantity>   ModifyRequest::getNewOriginalQty()   const { return newOriginalQty; }

RejectionReason ModifyRequest::validate() const {
    if (targetOrderID == 0) {
        return RejectionReason::InvalidModifyOrder;
    }
    if (!newPriceTicks.has_value() && !newOriginalQty.has_value()) {
        return RejectionReason::NoOpModify;
    }
    if (newPriceTicks.has_value() && *newPriceTicks <= 0) {
        return RejectionReason::InvalidModifyOrder;
    }
    if (newOriginalQty.has_value() && *newOriginalQty <= 0) {
        return RejectionReason::InvalidModifyOrder;
    }
    return RejectionReason::None;
}
