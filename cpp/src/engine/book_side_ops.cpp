#include "engine/book_side_ops.hpp"
#include <iterator>

namespace book_side_ops {

template <typename BookSide>
RejectionReason addToSide(BookSide& side, OrderIDMap& orderIDMap, const OrderPtr& order) {
    auto& level = side[order->getPriceTicks()];
    level.push_back(order);
    orderIDMap.emplace(order->getOrderID(), std::prev(level.end()));
    return RejectionReason::None;
}

template <typename BookSide>
RejectionReason removeFromSide(
    BookSide& side, OrderIDMap& orderIDMap, const OrderPtr& order, OrderIDMap::iterator orderIt
) {
    auto bookIt = side.find(order->getPriceTicks());
    if (bookIt == side.end()) {
        return RejectionReason::OrderBookInvariantViolation;
    }
    bookIt->second.erase(orderIt->second);
    if (bookIt->second.empty()) {
        side.erase(bookIt);
    }
    orderIDMap.erase(orderIt);
    return RejectionReason::None;
}

template <typename BookSide>
OrderPtr frontOfSide(const BookSide& side) {
    if (side.empty() || side.begin()->second.empty()) {
        return nullptr;
    }
    return side.begin()->second.front();
}

template <typename BookSide>
void popFrontOfSide(BookSide& side, OrderIDMap& orderIDMap) {
    if (side.empty()) {
        return;
    }
    auto& frontLevel = side.begin()->second;
    OrderPtr frontOrder = frontLevel.front();
    orderIDMap.erase(frontOrder->getOrderID());
    frontLevel.pop_front();
    if (frontLevel.empty()) {
        side.erase(side.begin());
    }
}

template <typename BookSide>
bool isFOKFillableOnSide(const BookSide& side, const OrderPtr& order, STPDecision stpDecision) {
    Quantity neededQty = order->getQty();
    OwnerID incomingOwnerID = order->getOwnerID();
    bool isLimit = order->getType() == OrderType::Limit;
    Quantity availableQty = 0;

    for (const auto& [price, ordersAtLevel] : side) {
        if (isLimit && side.key_comp()(order->getPriceTicks(), price)) {
            break;
        }
        for (const auto& restingOrder : ordersAtLevel) {
            if (restingOrder->getOwnerID() == incomingOwnerID) {
                if (stpDecision.cancelIncoming) {
                    return false;
                }
                continue;
            }
            availableQty += restingOrder->getQty();
            if (availableQty >= neededQty) {
                return true;
            }
        }
    }
    return false;
}

template <typename BookSide>
void summariseSide(
    const BookSide& side, std::size_t depthLimit, SideSummary& summary, std::vector<LevelInfo>& depths
) {
    summary.totalQuantity = 0;
    summary.totalOrderCount = 0;
    summary.totalNotionalValue = 0;
    depths.clear();

    std::size_t levelsAdded = 0;
    for (const auto& [price, ordersAtLevel] : side) {
        AggregateQuantity levelQty = 0;
        for (const auto& order : ordersAtLevel) {
            levelQty += static_cast<AggregateQuantity>(order->getQty());
        }
        summary.totalQuantity += levelQty;
        summary.totalOrderCount += static_cast<Count>(ordersAtLevel.size());
        summary.totalNotionalValue += static_cast<AggregateValue>(price) * levelQty;
        if (levelsAdded < depthLimit) {
            depths.push_back(LevelInfo{price, levelQty, static_cast<Count>(ordersAtLevel.size())});
            ++levelsAdded;
        }
    }
}

template RejectionReason addToSide<BidStructure>(BidStructure&, OrderIDMap&, const OrderPtr&);
template RejectionReason addToSide<AskStructure>(AskStructure&, OrderIDMap&, const OrderPtr&);

template RejectionReason removeFromSide<BidStructure>(
    BidStructure&, OrderIDMap&, const OrderPtr&, OrderIDMap::iterator
);
template RejectionReason removeFromSide<AskStructure>(
    AskStructure&, OrderIDMap&, const OrderPtr&, OrderIDMap::iterator
);

template OrderPtr frontOfSide<BidStructure>(const BidStructure&);
template OrderPtr frontOfSide<AskStructure>(const AskStructure&);

template void popFrontOfSide<BidStructure>(BidStructure&, OrderIDMap&);
template void popFrontOfSide<AskStructure>(AskStructure&, OrderIDMap&);

template bool isFOKFillableOnSide<BidStructure>(const BidStructure&, const OrderPtr&, STPDecision);
template bool isFOKFillableOnSide<AskStructure>(const AskStructure&, const OrderPtr&, STPDecision);

template void summariseSide<BidStructure>(
    const BidStructure&, std::size_t, SideSummary&, std::vector<LevelInfo>&
);
template void summariseSide<AskStructure>(
    const AskStructure&, std::size_t, SideSummary&, std::vector<LevelInfo>&
);

}  // namespace book_side_ops