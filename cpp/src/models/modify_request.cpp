#include "models/modify_request.hpp"

ModifyRequest::ModifyRequest(
    RequestID requestId_,
    OwnerID ownerID_,
    Timestamp timestamp_,
    OrderID targetOrderID_,
    std::optional<PriceTicks> newPriceTicks_,
    std::optional<Quantity> newQty_
)
:   IRequest(requestId_, RequestType::Modify, ownerID_, timestamp_),
    targetOrderID(targetOrderID_),
    newPriceTicks(newPriceTicks_),
    newQty(newQty_) {}

OrderID                   ModifyRequest::getTargetOrderID() const { return targetOrderID; }
std::optional<PriceTicks> ModifyRequest::getNewPriceTicks() const { return newPriceTicks; }
std::optional<Quantity>   ModifyRequest::getNewQty()        const { return newQty; }

RejectionReason ModifyRequest::validate() const {
    if (targetOrderID == 0) {
        return RejectionReason::InvalidModifyOrder;
    }
    if (!newPriceTicks.has_value() && !newQty.has_value()) {
        return RejectionReason::InvalidModifyOrder;
    }
    if (newPriceTicks.has_value() && *newPriceTicks <= 0) {
        return RejectionReason::InvalidModifyOrder;
    }
    if (newQty.has_value() && *newQty <= 0) {
        return RejectionReason::InvalidModifyOrder;
    }
    return RejectionReason::None;
}
