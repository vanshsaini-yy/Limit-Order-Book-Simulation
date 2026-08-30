#pragma once

#include <cstddef>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include "models/order.hpp"
#include "models/rejection_reason.hpp"
#include "models/market_structure_snapshot.hpp"

using OrderIDMap = std::unordered_map<OrderID, std::list<OrderPtr>::iterator>;
using BidStructure = std::map<PriceTicks, std::list<OrderPtr>, std::greater<PriceTicks>>;
using AskStructure = std::map<PriceTicks, std::list<OrderPtr>>;

namespace book_side_ops {

template <typename BookSide>
RejectionReason addToSide(BookSide& side, OrderIDMap& orderIDMap, const OrderPtr& order);

template <typename BookSide>
OrderPtr frontOfSide(const BookSide& side);

template <typename BookSide>
void popFrontOfSide(BookSide& side, OrderIDMap& orderIDMap);

template <typename BookSide>
bool isFOKFillableOnSide(const BookSide& side, const OrderPtr& order);

template <typename BookSide>
void summariseSide(
    const BookSide& side, std::size_t depthLimit, SideSummary& summary, std::vector<LevelInfo>& depths
);

}  // namespace book_side_ops