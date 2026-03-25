#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <cassert>

#include "models/matching_engine.hpp"
#include "models/order_book.hpp"
#include "models/order.hpp"
#include "policy/self_trade_prevention.hpp"

struct LatencySummary {
    double meanNs = 0.0;
    double medianNs = 0.0;
    double percentile95Ns = 0.0;
};

template <typename Fn>
double measureBatchNsPerOp(std::size_t iterations, Fn&& fn) {
    using Clock = std::chrono::steady_clock;
    auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        fn(i);
    }
    auto end = Clock::now();
    const auto totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return iterations == 0 ? 0.0 : static_cast<double>(totalNs) / static_cast<double>(iterations);
}

LatencySummary summarizeTrials(std::vector<double> samplesNsPerOp) {
    LatencySummary summary;
    if (samplesNsPerOp.empty()) {
        return summary;
    }

    std::sort(samplesNsPerOp.begin(), samplesNsPerOp.end());
    const std::size_t size = samplesNsPerOp.size();
    const std::size_t medianIdx = size / 2;
    const std::size_t p95Idx = static_cast<std::size_t>(std::ceil(0.95 * static_cast<double>(size))) - 1;

    const double total = std::accumulate(samplesNsPerOp.begin(), samplesNsPerOp.end(), 0.0);
    summary.meanNs = total / static_cast<double>(size);
    summary.medianNs = samplesNsPerOp[medianIdx];
    summary.percentile95Ns = samplesNsPerOp[std::min(p95Idx, size - 1)];
    return summary;
}

double benchmarkAddOrders(std::size_t n) {
    LimitOrderBook book;
    CancelIncomingSTP stp;
    MatchingEngine engine(&stp, &book);

    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
        OrderID orderId = static_cast<OrderID>(i + 1);
        OwnerID ownerId = static_cast<OwnerID>(i + 1);
        PriceTicks price = static_cast<PriceTicks>(100 + (i % 10));
        Quantity qty = 10;
        Timestamp timestamp = static_cast<Timestamp>(i + 1);
        orders.push_back(std::make_unique<Order>(
            orderId,
            ownerId,
            price,
            qty,
            Side::Buy,
            OrderType::Limit,
            timestamp
        ));
    }

    return measureBatchNsPerOp(n, [&](std::size_t i) {
        assert(engine.matchOrder(orders[i].get()) == RejectionReason::None);
    });
}

double benchmarkExecuteOrders(std::size_t n) {
    LimitOrderBook book;
    CancelIncomingSTP stp;
    MatchingEngine engine(&stp, &book);

    std::vector<std::unique_ptr<Order>> restingLimitOrders;
    restingLimitOrders.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
        OrderID orderId = static_cast<OrderID>(i + 1);
        OwnerID ownerId = static_cast<OwnerID>(i + 1);
        PriceTicks price = static_cast<PriceTicks>(100 + (i % 10));
        Quantity qty = 10;
        Timestamp timestamp = static_cast<Timestamp>(i + 1);
        restingLimitOrders.push_back(std::make_unique<Order>(
            orderId,
            ownerId,
            price,
            qty,
            Side::Buy,
            OrderType::Limit,
            timestamp
        ));
        assert(engine.matchOrder(restingLimitOrders.back().get()) == RejectionReason::None);
    }

    std::vector<std::unique_ptr<Order>> incomingMarketOrders;
    incomingMarketOrders.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        OrderID orderId = static_cast<OrderID>(n + i + 1);
        OwnerID ownerId = static_cast<OwnerID>(n + i + 1);
        Quantity qty = 10;
        Timestamp timestamp = static_cast<Timestamp>(n + i + 1);
        incomingMarketOrders.push_back(std::make_unique<Order>(
            orderId,
            ownerId,
            0,
            qty,
            Side::Sell,
            OrderType::Market,
            timestamp
        ));
    }

    return measureBatchNsPerOp(n, [&](std::size_t i) {
        assert(engine.matchOrder(incomingMarketOrders[i].get()) == RejectionReason::None);
    });
}

double benchmarkCancelOrders(std::size_t n) {
    LimitOrderBook book;
    CancelIncomingSTP stp;
    MatchingEngine engine(&stp, &book);

    std::vector<std::unique_ptr<Order>> restingLimitOrders;
    restingLimitOrders.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
        OrderID orderId = static_cast<OrderID>(i + 1);
        OwnerID ownerId = static_cast<OwnerID>(i + 1);
        PriceTicks price = static_cast<PriceTicks>(100 + (i % 10));
        Quantity qty = 10;
        Timestamp timestamp = static_cast<Timestamp>(i + 1);
        restingLimitOrders.push_back(std::make_unique<Order>(
            orderId,
            ownerId,
            price,
            qty,
            Side::Buy,
            OrderType::Limit, 
            timestamp
        ));
        assert(engine.matchOrder(restingLimitOrders.back().get()) == RejectionReason::None);
    }

    std::vector<std::unique_ptr<Order>> incomingCancelOrders;
    incomingCancelOrders.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        OrderID orderId = static_cast<OrderID>(n + i + 1);
        OwnerID ownerId = static_cast<OwnerID>(i + 1);
        Timestamp timestamp = static_cast<Timestamp>(n + i + 1);
        OrderID linkedId = static_cast<OrderID>(i + 1);
        incomingCancelOrders.push_back(std::make_unique<Order>(
            orderId,
            ownerId,
            0,
            0,
            Side::None,
            OrderType::Cancel,
            timestamp,
            linkedId
        ));
    }

    return measureBatchNsPerOp(n, [&](std::size_t i) {
        assert(engine.matchOrder(incomingCancelOrders[i].get()) == RejectionReason::None);
    });
}

template <typename BenchmarkFn>
LatencySummary runTrials(std::size_t n, std::size_t trials, BenchmarkFn&& benchmarkFn) {
    std::vector<double> samplesNsPerOp;
    samplesNsPerOp.reserve(trials);

    for (std::size_t t = 0; t < trials; ++t) {
        samplesNsPerOp.push_back(benchmarkFn(n));
    }

    return summarizeTrials(std::move(samplesNsPerOp));
}

void printSummary(const char* label, const LatencySummary& summary) {
    std::cout << label
              << " mean/median/p95: "
              << summary.meanNs << " / "
              << summary.medianNs << " / "
              << summary.percentile95Ns << " ns/op\n";
}

int main(int argc, char** argv) {
    std::size_t n = 1000000;
    std::size_t trials = 20;
    if (argc > 1) {
        n = static_cast<std::size_t>(std::stoul(argv[1]));
    }
    if (argc > 2) {
        trials = static_cast<std::size_t>(std::stoul(argv[2]));
    }
    if (trials == 0) {
        throw std::invalid_argument("trials must be >= 1");
    }

    LatencySummary addStats = runTrials(n, trials, benchmarkAddOrders);
    LatencySummary execStats = runTrials(n, trials, benchmarkExecuteOrders);
    LatencySummary cancelStats = runTrials(n, trials, benchmarkCancelOrders);

    std::cout << "MatchingEngine benchmark (iterations = " << n
              << ", trials = " << trials << ")\n";
    printSummary("Add Order", addStats);
    printSummary("Execute Order", execStats);
    printSummary("Cancel Order", cancelStats);

    return 0;
}
