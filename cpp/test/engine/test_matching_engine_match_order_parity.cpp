#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <vector>
#include "engine/matching_engine.hpp"
#include "models/new_order_request.hpp"

namespace {
struct OrderSpec {
    OrderID orderID;
    OwnerID ownerID;
    PriceTicks priceTicks;
    Quantity qty;
    Side side;
    OrderType type;
    Timestamp timestamp;
    TimeInForce timeInForce = TimeInForce::GTC;
    bool postOnly = false;
};

OrderSpec limit(
    OrderID orderID,
    OwnerID ownerID,
    PriceTicks priceTicks,
    Quantity qty,
    Side side,
    TimeInForce timeInForce = TimeInForce::GTC,
    bool postOnly = false
) {
    return OrderSpec{orderID, ownerID, priceTicks, qty, side, OrderType::Limit, 1000 + orderID, timeInForce, postOnly};
}

OrderSpec market(OrderID orderID, OwnerID ownerID, Quantity qty, Side side) {
    return OrderSpec{orderID, ownerID, 0, qty, side, OrderType::Market, 1000 + orderID};
}
}

class MatchingEngineMatchOrderParityTest : public ::testing::Test {
protected:
    CancelBothSTP stpPolicy;
    LimitOrderBook matchOrderBook;
    LimitOrderBook submitBook;
    std::optional<MatchingEngine> matchOrderEngine;
    std::optional<MatchingEngine> submitEngine;

    void makeEngines(std::optional<PriceTicks> deviation = std::nullopt) {
        matchOrderEngine.emplace(&matchOrderBook, &stpPolicy, nullptr, nullptr, deviation);
        submitEngine.emplace(&submitBook, &stpPolicy, nullptr, nullptr, deviation);
    }

    void expectSameSide(const SideSummary& expected, const SideSummary& actual) {
        EXPECT_EQ(actual.totalQuantity, expected.totalQuantity);
        EXPECT_EQ(actual.totalOrderCount, expected.totalOrderCount);
        EXPECT_EQ(actual.totalNotionalValue, expected.totalNotionalValue);
    }

    void expectSameLevels(const std::vector<LevelInfo>& expected, const std::vector<LevelInfo>& actual) {
        ASSERT_EQ(actual.size(), expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            EXPECT_EQ(actual[i].price, expected[i].price);
            EXPECT_EQ(actual[i].totalQuantity, expected[i].totalQuantity);
            EXPECT_EQ(actual[i].orderCount, expected[i].orderCount);
        }
    }

    void expectSameBooks() {
        MarketStructureSnapshot expected = matchOrderBook.snapshot(0, 1000);
        MarketStructureSnapshot actual = submitBook.snapshot(0, 1000);
        EXPECT_EQ(actual.bestBid, expected.bestBid);
        EXPECT_EQ(actual.bestAsk, expected.bestAsk);
        EXPECT_EQ(actual.spread, expected.spread);
        EXPECT_EQ(actual.mid, expected.mid);
        expectSameSide(expected.bidSummary, actual.bidSummary);
        expectSameSide(expected.askSummary, actual.askSummary);
        expectSameLevels(expected.bidDepths, actual.bidDepths);
        expectSameLevels(expected.askDepths, actual.askDepths);
        EXPECT_EQ(actual.tempo.tradeExecutionCount, expected.tempo.tradeExecutionCount);
        EXPECT_EQ(actual.tempo.orderCancellationCount, expected.tempo.orderCancellationCount);
        EXPECT_EQ(actual.tempo.totalVolumeTraded, expected.tempo.totalVolumeTraded);
        EXPECT_EQ(submitEngine->getLastTradedPrice(), matchOrderEngine->getLastTradedPrice());
    }

    void expectParity(const std::vector<OrderSpec>& specs) {
        std::vector<OrderPtr> matchOrderOrders;
        std::vector<SubmitResult> submitResults;

        for (const OrderSpec& spec : specs) {
            OrderPtr order = std::make_shared<Order>(
                spec.orderID, spec.ownerID, spec.priceTicks, spec.qty, spec.side, spec.type,
                spec.timestamp, spec.timeInForce, spec.postOnly
            );
            RejectionReason matchOrderReason = matchOrderEngine->matchOrder(order);

            NewOrderRequest request(
                spec.orderID, spec.ownerID, spec.timestamp, spec.orderID, spec.priceTicks, spec.qty,
                spec.side, spec.type, spec.timeInForce, spec.postOnly
            );
            SubmitResult result = submitEngine->submit(request);

            EXPECT_EQ(result.reason, matchOrderReason) << "order " << spec.orderID;
            matchOrderOrders.push_back(order);
            submitResults.push_back(result);
        }

        for (std::size_t i = 0; i < specs.size(); ++i) {
            if (submitResults[i].order == nullptr) {
                continue;
            }
            EXPECT_EQ(submitResults[i].order->getStatus(), matchOrderOrders[i]->getStatus()) << "order " << specs[i].orderID;
            EXPECT_EQ(submitResults[i].order->getQty(), matchOrderOrders[i]->getQty()) << "order " << specs[i].orderID;
            EXPECT_EQ(submitResults[i].order->getFilledQty(), matchOrderOrders[i]->getFilledQty()) << "order " << specs[i].orderID;
        }

        expectSameBooks();
    }
};

TEST_F(MatchingEngineMatchOrderParityTest, RestingLimit) {
    makeEngines();
    expectParity({limit(1, 1, 100, 10, Side::Buy)});
}

TEST_F(MatchingEngineMatchOrderParityTest, CrossingLimit_FullFill) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 5, Side::Sell),
        limit(2, 2, 100, 5, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, CrossingLimit_PartialFill_RemainderRests) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 5, Side::Sell),
        limit(2, 2, 100, 8, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, CrossingLimit_PartiallyFillsRestingOrder) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 10, Side::Sell),
        limit(2, 2, 100, 4, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, FIFOWithinLevel) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 2, Side::Sell),
        limit(2, 2, 100, 2, Side::Sell),
        limit(3, 3, 100, 2, Side::Sell),
        limit(4, 4, 100, 5, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, MarketOrder_SweepsTwoLevels) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 3, Side::Sell),
        limit(2, 2, 101, 4, Side::Sell),
        market(3, 3, 5, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, MarketOrder_OnEmptyBook) {
    makeEngines();
    expectParity({market(1, 1, 5, Side::Buy)});
}

TEST_F(MatchingEngineMatchOrderParityTest, IOC_RemainderDiscarded) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 3, Side::Sell),
        limit(2, 2, 100, 5, Side::Buy, TimeInForce::IOC),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, FOK_Fillable) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 5, Side::Sell),
        limit(2, 2, 100, 5, Side::Buy, TimeInForce::FOK),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, FOK_NotFillable) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 3, Side::Sell),
        limit(2, 2, 100, 5, Side::Buy, TimeInForce::FOK),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, PostOnly_WouldCross) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 5, Side::Sell),
        limit(2, 2, 100, 10, Side::Buy, TimeInForce::GTC, true),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, PostOnly_DoesNotCross_Rests) {
    makeEngines();
    expectParity({
        limit(1, 1, 105, 5, Side::Sell),
        limit(2, 2, 100, 10, Side::Buy, TimeInForce::GTC, true),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, SelfTrade_CancelBoth) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 5, Side::Sell),
        limit(2, 1, 100, 5, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, SelfTrade_AfterPartialFill) {
    makeEngines();
    expectParity({
        limit(1, 2, 100, 3, Side::Sell),
        limit(2, 1, 100, 5, Side::Sell),
        limit(3, 1, 100, 8, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, PriceCollar_RejectedAndAccepted) {
    makeEngines(5);
    expectParity({
        limit(1, 1, 100, 10, Side::Buy),
        limit(2, 2, 110, 10, Side::Sell),
        limit(3, 3, 200, 5, Side::Buy),
        limit(4, 4, 95, 5, Side::Buy),
        limit(5, 5, 110, 4, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, DuplicateOrderID) {
    makeEngines();
    expectParity({
        limit(1, 1, 100, 10, Side::Buy),
        limit(1, 2, 105, 3, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, InvalidFields_AreRejectedWithTheSameReason) {
    makeEngines();
    OrderSpec marketWithPrice = market(4, 1, 5, Side::Buy);
    marketWithPrice.priceTicks = 100;
    expectParity({
        limit(1, 1, 100, 0, Side::Buy),
        limit(0, 1, 100, 10, Side::Buy),
        limit(2, 1, 0, 10, Side::Buy),
        limit(3, 1, 100, 10, Side::Buy, TimeInForce::IOC, true),
        marketWithPrice,
        limit(5, 1, 100, 10, Side::Buy),
    });
}

TEST_F(MatchingEngineMatchOrderParityTest, DeterministicMixedFlow) {
    std::mt19937 generator(20261012);
    std::uniform_int_distribution<int> ownerPick(1, 5);
    std::uniform_int_distribution<int> pricePick(95, 105);
    std::uniform_int_distribution<int> qtyPick(1, 8);
    std::uniform_int_distribution<int> kindPick(0, 99);

    std::vector<OrderSpec> specs;
    for (OrderID id = 1; id <= 400; ++id) {
        OwnerID owner = static_cast<OwnerID>(ownerPick(generator));
        Side side = (generator() % 2 == 0) ? Side::Buy : Side::Sell;
        Quantity qty = qtyPick(generator);
        PriceTicks price = pricePick(generator);
        int kind = kindPick(generator);

        if (kind < 15) {
            specs.push_back(market(id, owner, qty, side));
        } else if (kind < 25) {
            specs.push_back(limit(id, owner, price, qty, side, TimeInForce::IOC));
        } else if (kind < 33) {
            specs.push_back(limit(id, owner, price, qty, side, TimeInForce::FOK));
        } else if (kind < 43) {
            specs.push_back(limit(id, owner, price, qty, side, TimeInForce::GTC, true));
        } else if (kind < 45) {
            specs.push_back(limit(id % 7 + 1, owner, price, qty, side));
        } else {
            specs.push_back(limit(id, owner, price, qty, side));
        }
    }

    makeEngines();
    expectParity(specs);

    EXPECT_GT(matchOrderBook.getTradeExecutionCount(), 50u);
}

TEST_F(MatchingEngineMatchOrderParityTest, DeterministicMixedFlow_WithPriceCollar) {
    std::mt19937 generator(20261013);
    std::uniform_int_distribution<int> ownerPick(1, 5);
    std::uniform_int_distribution<int> pricePick(85, 115);
    std::uniform_int_distribution<int> qtyPick(1, 8);
    std::uniform_int_distribution<int> kindPick(0, 99);

    std::vector<OrderSpec> specs;
    specs.push_back(limit(1, 1, 99, 5, Side::Buy));
    specs.push_back(limit(2, 2, 101, 5, Side::Sell));
    for (OrderID id = 3; id <= 400; ++id) {
        OwnerID owner = static_cast<OwnerID>(ownerPick(generator));
        Side side = (generator() % 2 == 0) ? Side::Buy : Side::Sell;
        Quantity qty = qtyPick(generator);
        PriceTicks price = pricePick(generator);
        int kind = kindPick(generator);

        if (kind < 15) {
            specs.push_back(market(id, owner, qty, side));
        } else if (kind < 25) {
            specs.push_back(limit(id, owner, price, qty, side, TimeInForce::IOC));
        } else if (kind < 35) {
            specs.push_back(limit(id, owner, price, qty, side, TimeInForce::GTC, true));
        } else {
            specs.push_back(limit(id, owner, price, qty, side));
        }
    }

    makeEngines(6);
    expectParity(specs);

    EXPECT_GT(matchOrderBook.getTradeExecutionCount(), 20u);
}
