#include <gtest/gtest.h>
#include <algorithm>
#include <thread>
#include <vector>
#include "infra/monotonic_trade_id_generator.hpp"

TEST(MonotonicTradeIdGeneratorTest, ReturnsSequentialIdsStartingFromDefault) {
    MonotonicTradeIdGenerator generator;
    EXPECT_EQ(generator.nextId(), 1u);
    EXPECT_EQ(generator.nextId(), 2u);
    EXPECT_EQ(generator.nextId(), 3u);
}

TEST(MonotonicTradeIdGeneratorTest, RespectsCustomStartId) {
    MonotonicTradeIdGenerator generator(100);
    EXPECT_EQ(generator.nextId(), 100u);
    EXPECT_EQ(generator.nextId(), 101u);
}

TEST(MonotonicTradeIdGeneratorTest, IsThreadSafeUnderConcurrentAccess) {
    MonotonicTradeIdGenerator generator(1);
    constexpr size_t threadCount = 4;
    constexpr size_t idsPerThread = 1000;

    std::vector<TradeID> ids;
    ids.reserve(threadCount * idsPerThread);
    std::vector<std::thread> threads;
    std::mutex idsMutex;

    for (size_t t = 0; t < threadCount; ++t) {
        threads.emplace_back([&]() {
            std::vector<TradeID> local;
            local.reserve(idsPerThread);
            for (size_t i = 0; i < idsPerThread; ++i) {
                local.push_back(generator.nextId());
            }
            std::lock_guard<std::mutex> lock(idsMutex);
            ids.insert(ids.end(), local.begin(), local.end());
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    EXPECT_EQ(ids.size(), threadCount * idsPerThread);
}
