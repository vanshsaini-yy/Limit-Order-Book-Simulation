#include "matching_engine_facade_bindings.hpp"

#include <pybind11/stl.h>

#include "matching_engine_facade.hpp"

namespace py = pybind11;

namespace {
py::dict levelToDict(const LevelInfo& level) {
    py::dict levelDict;
    levelDict["price_ticks"] = level.price;
    levelDict["total_quantity"] = level.totalQuantity;
    levelDict["order_count"] = level.orderCount;
    return levelDict;
}

py::list levelsToList(const std::vector<LevelInfo>& levels) {
    py::list output;
    for (const LevelInfo& level : levels) {
        output.append(levelToDict(level));
    }
    return output;
}

py::dict snapshotToDict(const MarketStructureSnapshot& snapshot) {
    py::dict output;

    output["timestamp"] = snapshot.timestamp;
    output["best_bid"] = snapshot.bestBid.has_value() ? py::cast(*snapshot.bestBid) : py::none();
    output["best_ask"] = snapshot.bestAsk.has_value() ? py::cast(*snapshot.bestAsk) : py::none();
    output["spread"] = snapshot.spread.has_value() ? py::cast(*snapshot.spread) : py::none();
    output["mid"] = snapshot.mid.has_value() ? py::cast(*snapshot.mid) : py::none();

    py::dict bidSummary;
    bidSummary["total_quantity"] = snapshot.bidSummary.totalQuantity;
    bidSummary["order_count"] = snapshot.bidSummary.orderCount;
    bidSummary["total_notional_value"] = snapshot.bidSummary.totalNotionalValue;

    py::dict askSummary;
    askSummary["total_quantity"] = snapshot.askSummary.totalQuantity;
    askSummary["order_count"] = snapshot.askSummary.orderCount;
    askSummary["total_notional_value"] = snapshot.askSummary.totalNotionalValue;

    py::dict tempo;
    tempo["trade_execution_count"] = snapshot.tempo.tradeExecutionCount;
    tempo["order_cancellation_count"] = snapshot.tempo.orderCancellationCount;
    tempo["total_volume_traded"] = snapshot.tempo.totalVolumeTraded;

    output["bid_summary"] = bidSummary;
    output["ask_summary"] = askSummary;
    output["bid_depths"] = levelsToList(snapshot.bidDepths);
    output["ask_depths"] = levelsToList(snapshot.askDepths);
    output["tempo"] = tempo;

    return output;
}
}

void bindMatchingEngineFacade(py::module_& module) {
    py::class_<MatchingEngineFacade>(module, "MatchingEngineFacade")
        .def(
            py::init<const std::string&, const std::string&, const std::string&, const std::string&, TradeID>(),
            py::arg("stp_policy"),
            py::arg("trade_id_generator") = "none",
            py::arg("trade_logger") = "none",
            py::arg("trade_log_file_path") = "trades.bin",
            py::arg("trade_id_start") = 1
        )
        .def("match_order", &MatchingEngineFacade::matchOrder, py::arg("order"))
        .def(
            "snapshot",
            [](const MatchingEngineFacade& facade, Timestamp now, std::size_t depthLimit) {
                return snapshotToDict(facade.snapshot(now, depthLimit));
            },
            py::arg("now"),
            py::arg("depth_limit") = 5
        );
}
