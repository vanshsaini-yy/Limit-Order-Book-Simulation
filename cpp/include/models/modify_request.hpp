#pragma once
#include <optional>
#include "models/request.hpp"

class ModifyRequest : public IRequest {
private:
    OrderID targetOrderID;
    std::optional<PriceTicks> newPriceTicks;
    std::optional<Quantity> newQty;

public:
    ModifyRequest(
        RequestID requestId_,
        OwnerID ownerID_,
        Timestamp timestamp_,
        OrderID targetOrderID_,
        std::optional<PriceTicks> newPriceTicks_,
        std::optional<Quantity> newQty_
    );

    OrderID                   getTargetOrderID() const;
    std::optional<PriceTicks> getNewPriceTicks() const;
    std::optional<Quantity>   getNewQty()        const;

    RejectionReason validate() const override;
};
