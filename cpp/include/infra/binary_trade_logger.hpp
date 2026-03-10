#pragma once
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include "infra/trade_logger.hpp"

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

        static TradeLogRecord toRecord(const Trade& trade) {
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

    public:
        explicit BinaryTradeLogger(const std::string& filePath)
            : out(filePath, std::ios::out | std::ios::app | std::ios::binary) {
            if (!out.is_open()) {
                throw std::runtime_error("Failed to open binary trade log file");
            }
        }

        ~BinaryTradeLogger() override {
            if (out.is_open()) {
                out.flush();
                out.close();
            }
        }

        void log(const Trade& trade) override {
            TradeLogRecord record = toRecord(trade);
            out.write(reinterpret_cast<const char*>(&record), sizeof(TradeLogRecord));
        }

        void flush() override {
            out.flush();
        }

        void close() override {
            if (out.is_open()) {
                out.close();
            }
        }
};
