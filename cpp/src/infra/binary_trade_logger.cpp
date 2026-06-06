#include "infra/binary_trade_logger.hpp"

TradeLogRecord BinaryTradeLogger::toRecord(const Trade& trade) {
    return TradeLogRecord{
        trade.getTradeID(),
        trade.getTimestamp(),
        trade.getPriceTicks(),
        trade.getTakerOrderID(),
        trade.getMakerOrderID(),
        trade.getQty(),
        static_cast<uint8_t>(trade.getSide()),
        {}
    };
}

BinaryTradeLogger::BinaryTradeLogger(const std::string& filePath)
    : out(filePath, std::ios::out | std::ios::app | std::ios::binary) {
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open binary trade log file");
    }
}

BinaryTradeLogger::~BinaryTradeLogger() {
    if (out.is_open()) {
        out.flush();
        out.close();
    }
}

void BinaryTradeLogger::log(const Trade& trade) {
    TradeLogRecord record = toRecord(trade);
    out.write(reinterpret_cast<const char*>(&record), sizeof(TradeLogRecord));
}

void BinaryTradeLogger::flush() {
    out.flush();
}

void BinaryTradeLogger::close() {
    if (out.is_open()) {
        out.close();
    }
}
