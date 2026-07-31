#include "matching_engine_facade_bindings.hpp"
#include "matching_engine_facade.hpp"

#include <pybind11/stl.h>

namespace py = pybind11;

namespace {
py::dict levelToDict(const LevelInfo& level, double tickSize, double lotSize) {
    py::dict levelDict;
    levelDict["price"] = level.price * tickSize;
    levelDict["total_quantity"] = level.totalQuantity * lotSize;
    levelDict["order_count"] = level.orderCount;
    return levelDict;
}

py::list levelsToList(const std::vector<LevelInfo>& levels, double tickSize, double lotSize) {
    py::list output;
    for (const LevelInfo& level : levels) {
        output.append(levelToDict(level, tickSize, lotSize));
    }
    return output;
}

py::dict snapshotToDict(const MarketStructureSnapshot& snapshot, double tickSize, double lotSize, double timeInterval) {
    py::dict output;

    output["timestamp"] = snapshot.timestamp * timeInterval;
    output["best_bid"] = snapshot.bestBid.has_value() ? py::cast(*snapshot.bestBid * tickSize) : py::none();
    output["best_ask"] = snapshot.bestAsk.has_value() ? py::cast(*snapshot.bestAsk * tickSize) : py::none();
    output["spread"] = snapshot.spread.has_value() ? py::cast(*snapshot.spread * tickSize) : py::none();
    output["mid"] = snapshot.mid.has_value() ? py::cast(*snapshot.mid * tickSize) : py::none();

    py::dict bidSummary;
    bidSummary["total_quantity"] = snapshot.bidSummary.totalQuantity * lotSize;
    bidSummary["order_count"] = snapshot.bidSummary.orderCount;
    bidSummary["total_notional_value"] = snapshot.bidSummary.totalNotionalValue * tickSize * lotSize;

    py::dict askSummary;
    askSummary["total_quantity"] = snapshot.askSummary.totalQuantity * lotSize;
    askSummary["order_count"] = snapshot.askSummary.orderCount;
    askSummary["total_notional_value"] = snapshot.askSummary.totalNotionalValue * tickSize * lotSize;

    py::dict tempo;
    tempo["trade_execution_count"] = snapshot.tempo.tradeExecutionCount;
    tempo["order_cancellation_count"] = snapshot.tempo.orderCancellationCount;
    tempo["total_volume_traded"] = snapshot.tempo.totalVolumeTraded * lotSize;

    output["bid_summary"] = bidSummary;
    output["ask_summary"] = askSummary;
    output["bid_depths"] = levelsToList(snapshot.bidDepths, tickSize, lotSize);
    output["ask_depths"] = levelsToList(snapshot.askDepths, tickSize, lotSize);
    output["tempo"] = tempo;

    return output;
}
}

void bindMatchingEngineFacade(py::module_& module) {
    py::class_<MatchingEngineFacade>(module, "MatchingEngine")
        .def(
            py::init<const std::string&, const std::string&, TradeID, const std::string&, const std::string&, double, double, double, std::optional<PriceTicks>>(),
            py::arg("stp_policy"),
            py::arg("trade_id_generator") = "none",
            py::arg("trade_id_start") = 1,
            py::arg("trade_logger") = "none",
            py::arg("trade_log_file_path") = "trades.bin",
            py::arg("tick_size") = DEFAULT_TICK_SIZE,
            py::arg("lot_size") = DEFAULT_LOT_SIZE,
            py::arg("time_interval") = DEFAULT_TIME_INTERVAL,
            py::arg("max_deviation_ticks") = std::nullopt
        )
        .def_property_readonly("tick_size",     &MatchingEngineFacade::getTickSize)
        .def_property_readonly("lot_size",      &MatchingEngineFacade::getLotSize)
        .def_property_readonly("time_interval", &MatchingEngineFacade::getTimeInterval)
        .def(
            "match_order",
            &MatchingEngineFacade::matchOrder,
            py::arg("order")
        )
        .def(
            "snapshot",
            [](const MatchingEngineFacade& facade, Timestamp now, std::size_t depthLimit) {
                return snapshotToDict(
                    facade.snapshot(now, depthLimit),
                    facade.getTickSize(),
                    facade.getLotSize(),
                    facade.getTimeInterval()
                );
            },
            py::arg("now"),
            py::arg("depth_limit") = 5
        );
}
