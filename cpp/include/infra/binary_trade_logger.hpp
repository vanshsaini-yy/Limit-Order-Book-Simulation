#pragma once
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include "infra/trade_logger.hpp"

// TODO: make priceTicks int32_t
struct TradeLogRecord {
    uint64_t tradeID;
    uint64_t timestamp;
    int64_t priceTicks;
    uint32_t takerOrderID;
    uint32_t makerOrderID;
    int32_t qty;
    uint8_t side;
    uint8_t padding[3]{};
};

class BinaryTradeLogger : public TradeLogger {
private:
    std::ofstream out;

    static TradeLogRecord toRecord(const Trade& trade);

public:
    explicit BinaryTradeLogger(const std::string& filePath);
    ~BinaryTradeLogger() override;

    void log(const Trade& trade) override;
    void flush() override;
    void close() override;
};
