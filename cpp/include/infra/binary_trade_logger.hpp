#pragma once
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include "infra/trade_logger.hpp"

struct TradeLogRecord {
    uint64_t timestamp;
    uint32_t tradeID;
    int32_t priceTicks;
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
