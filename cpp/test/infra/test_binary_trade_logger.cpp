#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "infra/binary_trade_logger.hpp"

class BinaryTradeLoggerTest : public ::testing::Test {
protected:
    std::filesystem::path tempPath;

    void TearDown() override {
        if (!tempPath.empty()) {
            std::filesystem::remove(tempPath);
        }
    }

    void createTempPath(const std::string& fileName) {
        tempPath = std::filesystem::temp_directory_path() / fileName;
        std::filesystem::remove(tempPath);
    }

    static TradeLogRecord readRecord(std::ifstream& in) {
        TradeLogRecord record{};
        in.read(reinterpret_cast<char*>(&record), sizeof(TradeLogRecord));
        return record;
    }
};

TEST_F(BinaryTradeLoggerTest, WritesSingleRecord) {
    createTempPath("trade_log_test.bin");

    Trade trade(1001, 11, 22, 12345, 50, Side::Buy, 1622547800);

    {
        BinaryTradeLogger logger(tempPath.string());
        logger.log(trade);
        logger.flush();
    }

    std::ifstream in(tempPath, std::ios::in | std::ios::binary);
    ASSERT_TRUE(in.is_open());

    TradeLogRecord record = readRecord(in);
    EXPECT_EQ(in.gcount(), static_cast<std::streamsize>(sizeof(TradeLogRecord)));

    EXPECT_EQ(record.tradeID, 1001u);
    EXPECT_EQ(record.timestamp, 1622547800u);
    EXPECT_EQ(record.priceTicks, 12345);
    EXPECT_EQ(record.takerOrderID, 11u);
    EXPECT_EQ(record.makerOrderID, 22u);
    EXPECT_EQ(record.qty, 50);
    EXPECT_EQ(record.side, static_cast<uint8_t>(Side::Buy));
}

TEST_F(BinaryTradeLoggerTest, WritesMultipleRecordsInOrder) {
    createTempPath("trade_log_test_multi.bin");

    Trade tradeA(2001, 21, 31, 5000, 10, Side::Sell, 1622547801);
    Trade tradeB(2002, 22, 32, 5100, 20, Side::Buy, 1622547802);

    {
        BinaryTradeLogger logger(tempPath.string());
        logger.log(tradeA);
        logger.log(tradeB);
        logger.flush();
    }

    std::ifstream in(tempPath, std::ios::in | std::ios::binary);
    ASSERT_TRUE(in.is_open());

    TradeLogRecord recordA = readRecord(in);
    TradeLogRecord recordB = readRecord(in);

    EXPECT_EQ(recordA.tradeID, 2001u);
    EXPECT_EQ(recordA.takerOrderID, 21u);
    EXPECT_EQ(recordA.makerOrderID, 31u);
    EXPECT_EQ(recordA.priceTicks, 5000);
    EXPECT_EQ(recordA.qty, 10);
    EXPECT_EQ(recordA.side, static_cast<uint8_t>(Side::Sell));
    EXPECT_EQ(recordA.timestamp, 1622547801u);

    EXPECT_EQ(recordB.tradeID, 2002u);
    EXPECT_EQ(recordB.takerOrderID, 22u);
    EXPECT_EQ(recordB.makerOrderID, 32u);
    EXPECT_EQ(recordB.priceTicks, 5100);
    EXPECT_EQ(recordB.qty, 20);
    EXPECT_EQ(recordB.side, static_cast<uint8_t>(Side::Buy));
    EXPECT_EQ(recordB.timestamp, 1622547802u);
}

TEST_F(BinaryTradeLoggerTest, LogDoesNotThrowOnCloseAfterFlush) {
    createTempPath("trade_log_test_close.bin");

    Trade trade(3001, 41, 51, 6000, 15, Side::Buy, 1622547803);

    BinaryTradeLogger logger(tempPath.string());
    logger.log(trade);
    logger.flush();
    EXPECT_NO_THROW(logger.close());

    std::ifstream in(tempPath, std::ios::in | std::ios::binary);
    ASSERT_TRUE(in.is_open());
    TradeLogRecord record = readRecord(in);
    EXPECT_EQ(record.tradeID, 3001u);
}

TEST_F(BinaryTradeLoggerTest, WritesAppendedRecords) {
    createTempPath("trade_log_test_append.bin");

    Trade tradeA(4001, 61, 71, 7000, 5, Side::Sell, 1622547804);
    Trade tradeB(4002, 62, 72, 7100, 6, Side::Buy, 1622547805);

    {
        BinaryTradeLogger logger(tempPath.string());
        logger.log(tradeA);
        logger.flush();
    }

    {
        BinaryTradeLogger logger(tempPath.string());
        logger.log(tradeB);
        logger.flush();
    }

    std::ifstream in(tempPath, std::ios::in | std::ios::binary);
    ASSERT_TRUE(in.is_open());

    TradeLogRecord recordA = readRecord(in);
    TradeLogRecord recordB = readRecord(in);

    EXPECT_EQ(recordA.tradeID, 4001u);
    EXPECT_EQ(recordB.tradeID, 4002u);
}
