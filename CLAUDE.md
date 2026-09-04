# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Test

**Build and run all tests (from repo root):**
```bash
./run.sh
```
This creates `build/`, runs CMake, compiles, and runs CTest.

**Manual build (C++ only):**
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

**Manual build (with Python bindings):**
```bash
cmake -S . -B build -DLOB_BUILD_PYTHON_BINDINGS=ON
cmake --build build
```

**Run all C++ tests:**
```bash
cd build && ctest --output-on-failure
```

**Run a single test binary directly** (all tests are compiled into one binary):
```bash
./build/cpp/tests --gtest_filter="TestSuiteName.TestName"
```

**Python simulation** (requires Python bindings compiled into `build/cpp/`):
```bash
source .venv/bin/activate
python simulate_noisy_traders.py
```

## Architecture

The project is a C++23 library (`lob_core`) with separate headers and `.cpp` implementations, compiled via CMake with GoogleTest for unit tests. Python bindings via pybind11 expose a `MatchingEngineFacade` for simulation.

### Component Hierarchy

```
MatchingEngine
├── LimitOrderBook        # price-level data structure
│   └── OrderValidator    # validates before add/remove
├── ExecutionEngine       # executes matched trades
├── STPPolicy             # self-trade prevention (pluggable)
├── TradeLogger           # optional trade persistence (pluggable)
└── TradeIdGenerator      # optional trade ID assignment (pluggable)
```

`OrderLifecycle` is a stateless utility used by the engine to derive the correct `OrderStatus` transition (Pending → PartiallyExecuted → Executed / Cancelled / CancelledAfterPartialExecution).

### Key Design Decisions

**`LimitOrderBook`** stores bids as `std::map<PriceTicks, std::list<OrderPtr>, std::greater<>>` (best bid first) and asks as `std::map<PriceTicks, std::list<OrderPtr>>` (best ask first). Each price level is a FIFO `std::list`, giving price-time priority. An `std::unordered_map<OrderID, list::iterator>` enables O(1) order lookup and cancellation. `getMidPrice()` and `getSpread()` return `std::optional<PriceTicks>`, `std::nullopt` unless both a best bid and best ask exist.

**`Order`** uses `shared_ptr` (`OrderPtr = std::shared_ptr<Order>`) because the book and the order's submitter are independent owners: `MatchingEngine` and `LimitOrderBook::cancelOrder` both read an order after `popFront`/erase has dropped the book's reference, a resting order can outlive the caller's handle, and pybind11 holds `Order` by `shared_ptr`. Signatures reflect this split: call sites that participate in ownership (inserting into or erasing from the book, or holding an order across a step that might erase it) take `const OrderPtr&` or `OrderPtr`; call sites that only read or mutate an already-owned `Order` — `ExecutionEngine::executeTrade`, `isSelfTrade`, `OrderValidator::validateLimitOrder`/`validateMarketOrder`/`validateCancelOrder`, `MatchingEngine::violatesPriceCollar` — take `Order&`/`const Order&` instead, so they neither pay the atomic refcount nor imply any ownership stake. The two `OrderValidator` entry points, `validateBeforeMatching` and `validateBeforeAddingOrRemoving`, stay on `const OrderPtr&` because they test for null before dispatching to the non-owning validators. Prices are integer ticks (`PriceTicks = int32_t`), not floats. `PriceTicks` and `Quantity` are signed on purpose — making them unsigned would wrap a negative input to a large positive value that passes `OrderValidator`'s `> 0` checks. All internal book logic operates in ticks; scaling to real-world units happens only at the Python API boundary. `TimeInForce` (`GTC`/`IOC`/`FOK`) and a `postOnly` bool are additional fields governing order-execution constraints.

**`Trade`** captures a completed fill: `TradeID`, taker/maker `OrderID`, `PriceTicks`, `Quantity`, `Side`, and `Timestamp`. Created by `ExecutionEngine` and optionally persisted via `TradeLogger`.

**`MatchingEngine::matchOrder`** validates the incoming order, then runs a sequence of pre-loop rejection checks in order — duplicate order ID, Post-Only-would-cross, FOK-insufficient-liquidity, then price-collar violation (see below) — before entering the matching loop. While the incoming order is marketable, it checks for self-trades (same `ownerID`), applies the `STPPolicy` if needed, calls `ExecutionEngine::executeTrade`, assigns a `TradeID` via `TradeIdGenerator` (if set), logs via `TradeLogger` (if set), and finally either places the remainder in the book (Limit orders, subject to `TimeInForce`) or discards it (Market orders, or IOC/FOK remainders).

**Price Collar** is an optional order-level circuit breaker: `MatchingEngine` holds `std::optional<PriceTicks> maxDeviationTicks` (constructor parameter, `std::nullopt` disables it) and `std::optional<PriceTicks> lastTradedPrice` (updated to the maker's price after every fill). A `Limit` order priced outside `[reference − maxDeviationTicks, reference + maxDeviationTicks]` is rejected with `RejectionReason::PriceCollarViolation`, where `reference` is `lastTradedPrice` if set, else `LimitOrderBook::getMidPrice()`, else the check is skipped. `Market` orders always bypass the collar.

**`STPPolicy`** is a polymorphic interface with three concrete implementations: `CancelBothSTP`, `CancelIncomingSTP`, `CancelRestingSTP`. Injected into `MatchingEngine` at construction. If applying the policy ends the incoming order's life (fully cancelled, or `CancelledAfterPartialExecution`), `matchOrder` returns `RejectionReason::SelfTradePrevention` instead of `None`. `CancelRestingSTP`, which only cancels the resting order and lets the incoming order continue matching or rest, still returns `None`.

**`TradeLogger`** / **`TradeIdGenerator`** are optional polymorphic interfaces injected into `MatchingEngine`. Concrete implementations: `BinaryTradeLogger` (writes fixed-width `TradeLogRecord` structs to a binary file) and `MonotonicTradeIdGenerator` (atomic counter). Pass `nullptr` to disable.

**`MarketStructureSnapshot`** is returned by `LimitOrderBook::snapshot()` and contains `bestBid`, `bestAsk`, `spread`, `mid` (all in `PriceTicks`), per-side `SideSummary` (quantity, order count, notional), per-level `bidDepths`/`askDepths` (`LevelInfo`), and `TempoMetrics` (trade count, cancel count, volume).

**`MatchingEngineFacade`** (Python-only) owns a `LimitOrderBook`, `MatchingEngine`, and all three infrastructure objects above. Constructed via string factory args (`"cancel_both"`, `"monotonic"`, `"binary"`, etc.) plus the three market-convention scalars. It is the single entry point for Python callers.

### Market Convention Scalars

All three are constructor parameters on `MatchingEngineFacade` (defaults in `cpp/include/utils/constants.hpp`). Internal C++ always works in raw integer units; the facade multiplies at output.

| Parameter | Default | Applied to |
|---|---|---|
| `tick_size` | `0.01` | `best_bid`, `best_ask`, `spread`, `mid`, depth `price`, `total_notional_value` |
| `lot_size` | `1.0` | `total_quantity` (summary + depth), `total_volume_traded`, `total_notional_value` |
| `time_interval` | `1.0` | `timestamp` |

`total_notional_value` is computed as `sum(price_ticks × qty_lots)` and scaled by `tick_size × lot_size` at output.

**Naming rule:** `price_ticks` always means a raw integer tick value (used on input — `Order` constructor and property). `price` always means a real-world scaled float (used in snapshot output). Never use them interchangeably.

### Source Layout

```
cpp/
├── include/
│   ├── models/         # Order, Trade, MarketStructureSnapshot, RejectionReason, enums (Side, OrderType, TimeInForce, OrderStatus)
│   ├── engine/         # LimitOrderBook, MatchingEngine, ExecutionEngine, book_side_ops
│   ├── policy/         # OrderValidator, OrderLifecycle, STPPolicy
│   ├── infra/          # TradeLogger, TradeIdGenerator (interfaces + concrete impls)
│   └── utils/          # order_utils.hpp (isSelfTrade helper), constants.hpp (tick/lot/time defaults)
├── src/                # .cpp implementations mirroring include/
├── test/               # GoogleTest files mirroring include/models, include/policy, include/infra
└── python_binding/     # MatchingEngineFacade + pybind11 bindings
googletest/             # git submodule
build/                  # CMake out-of-tree build (do not edit)
```

The Python API is fully documented in `PYTHON_LIBRARY.md`, including all enum values, `Order` validation rules, `MatchingEngine` constructor string options, and the `snapshot()` return dict schema.
