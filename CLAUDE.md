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
├── ModifyPolicy          # cancel-replace vs. in-place decision (pluggable)
├── TradeLogger           # optional trade persistence (pluggable)
└── TradeIdGenerator      # optional trade ID assignment (pluggable)
```

`OrderLifecycle` is a stateless utility used by the engine to derive the correct `OrderStatus` transition (Pending → PartiallyExecuted → Executed / Cancelled / CancelledAfterPartialExecution).

### Key Design Decisions

**`LimitOrderBook`** stores bids as `std::map<PriceTicks, std::list<OrderPtr>, std::greater<>>` (best bid first) and asks as `std::map<PriceTicks, std::list<OrderPtr>>` (best ask first). Each price level is a FIFO `std::list`, giving price-time priority. An `std::unordered_map<OrderID, list::iterator>` enables O(1) order lookup and cancellation. `getMidPrice()` and `getSpread()` return `std::optional<PriceTicks>`, `std::nullopt` unless both a best bid and best ask exist.

**`Order`** uses `shared_ptr` (`OrderPtr = std::shared_ptr<Order>`) because the book and the order's submitter are independent owners: `MatchingEngine` and `LimitOrderBook::cancelOrder` both read an order after `popFront` has dropped the book's reference, a resting order can outlive the caller's handle, and pybind11 holds `Order` by `shared_ptr`. Signatures reflect this split: call sites that participate in ownership (inserting into or erasing from the book, or holding an order across a step that might erase it) take `const OrderPtr&` or `OrderPtr`; call sites that only read or mutate an already-owned `Order` — `ExecutionEngine::executeTrade`, `isSelfTrade`, `OrderValidator::validateLimitOrder`/`validateMarketOrder`, `MatchingEngine::violatesPriceCollar` — take `Order&`/`const Order&` instead, so they neither pay the atomic refcount nor imply any ownership stake. The two `OrderValidator` entry points, `validateBeforeMatching` and `validateBeforeAddingOrRemoving`, stay on `const OrderPtr&` because they test for null before dispatching to the non-owning validators. Prices are integer ticks (`PriceTicks = int32_t`), not floats. `PriceTicks` and `Quantity` are signed on purpose — making them unsigned would wrap a negative input to a large positive value that passes `OrderValidator`'s `> 0` checks. All internal book logic operates in ticks; scaling to real-world units happens only at the Python API boundary. `TimeInForce` (`GTC`/`IOC`/`FOK`) and a `postOnly` bool are additional fields governing order-execution constraints. `Order` also tracks `originalQty` (set once, at construction, to the initial `qty`) alongside the mutable, remaining `qty`; `getFilledQty()` returns `originalQty - qty`. `modifyOriginalQty(Quantity newOriginalQty)` is the FIX cancel-replace primitive: it takes the new *total* order quantity (FIX `OrderQty`, tag 38) and recomputes `qty` as `newOriginalQty - getFilledQty()` (FIX `LeavesQty`, tag 151) so that `getFilledQty()` (FIX `CumQty`, tag 14) is unaffected by the call — it is not a fill, and must not be read as one. This is distinct from `reduceQty`, which represents an actual fill and does move `getFilledQty()`.

**`Trade`** captures a completed fill: `TradeID`, taker/maker `OrderID`, `PriceTicks`, `Quantity`, `Side`, and `Timestamp`. Created by `ExecutionEngine` and optionally persisted via `TradeLogger`.

**`MatchingEngine::matchOrder`** validates the incoming order, then runs a sequence of pre-loop rejection checks in order — duplicate order ID, Post-Only-would-cross, FOK-insufficient-liquidity, then price-collar violation (see below) — before entering the matching loop. While the incoming order is marketable, it checks for self-trades (same `ownerID`), applies the `STPPolicy` if needed, calls `ExecutionEngine::executeTrade`, assigns a `TradeID` via `TradeIdGenerator` (if set), logs via `TradeLogger` (if set), and finally either places the remainder in the book (Limit orders, subject to `TimeInForce`) or discards it (Market orders, or IOC/FOK remainders).

**`SubmitResult`** (`cpp/include/engine/submit_result.hpp`) is what both `submit(const CancelRequest&)` and `submit(const ModifyRequest&)` return: `{RejectionReason reason, OrderPtr order}`. `order` is the order the caller should now inspect, and it is null only where the engine has no order it is entitled to hand over: a failed `validate()`, and — deliberately — a missing order *or* an owner mismatch. `LimitOrderBook::getOrder` takes no `OwnerID`, so returning whatever it found would leak another owner's order and undo the non-leaking conflation described below; a non-owner must receive exactly `{OrderToBeCancelledDoesNotExist, nullptr}` (or `OrderToBeModifiedDoesNotExist`), identical to the response for an order that never existed. Rejections that fire *after* the owner check (`NoOpModify`, `ModifyQuantityBelowFilled`) return the resting order, since ownership is proven — for `ModifyQuantityBelowFilled` that is the only way the caller can see the `getFilledQty()` that caused it. On a successful cancel, `order` is the cancelled order, kept alive only by the returned `shared_ptr` (it has left the book). On a modify: the in-place and done-for-day branches return the resting order; a priority-losing modify that clears `checkBeforeMatching` returns the *replacement* (whatever `executeMatching` then did to it — rested, filled, or cancelled by STP); but a priority-losing modify rejected *by* `checkBeforeMatching` returns the **resting** order, never the replacement, because that replacement was never in the book and has been marked `Cancelled`, while the original is still live and untouched. `MatchingEngineFacade::cancel` still returns a bare `RejectionReason` (`.reason`); Python is unchanged until the next phase.

**`MatchingEngine::submit(const CancelRequest&)`** is the separate entry point for cancellation: it calls `request.validate()`, then `LimitOrderBook::cancelOrder(targetOrderID, ownerID)` (which does the existence check, the owner check — returning `RejectionReason::OrderToBeCancelledDoesNotExist` for both non-existence and ownership mismatch, deliberately, so a caller can't probe for another owner's order — and the `OrderLifecycle::afterCancelResting` status transition), then `LimitOrderBook::recordCancellation()` on success. `CancelRequest` (defined in `cpp/include/models/cancel_request.hpp`) is an immutable inbound message, not an `Order` — it is never stored in the book. It is one of three concrete types under the `IRequest` base (`cpp/include/models/request.hpp`), alongside `NewOrderRequest` and `ModifyRequest`; `CancelRequest` and `ModifyRequest` are wired into `MatchingEngine`, `NewOrderRequest` is not yet. New orders still flow through `matchOrder(const OrderPtr&)` — the entry points coexist until the request hierarchy fully replaces `Order`-based submission.

**`MatchingEngine::submit(const ModifyRequest&)`** implements FIX-style cancel-replace. After `request.validate()`, it looks up the resting order via `LimitOrderBook::getOrder(targetOrderID)` — a read-only lookup, unlike `cancelOrder` which removes — and rejects with `RejectionReason::OrderToBeModifiedDoesNotExist` for both a missing order and an owner mismatch, for the same non-leaking reason as cancel. `ModifyRequest::validate()` already returns `RejectionReason::NoOpModify` when neither a new price nor a new quantity is given; the engine extends that to a request whose given price and/or `newOriginalQty` equal the resting order's current price and `getOriginalQty()` (nothing would change), rejecting it with `NoOpModify` before any other decision so the order is left untouched and keeps its queue position. If `newOriginalQty` is given and below `getFilledQty()`, it rejects with `RejectionReason::ModifyQuantityBelowFilled`; if `newOriginalQty` equals `getFilledQty()` exactly, that is treated as "done for day" — the remainder is cancelled via `LimitOrderBook::cancelOrder` plus `recordCancellation()`, not as an error. Otherwise it asks the injected `ModifyPolicy` (`cpp/include/policy/modify_policy.hpp`) for a `ModifyDecision`. Two implementations ship: `DefaultModifyPolicy` says a price change or a quantity *increase* loses queue priority, a quantity decrease (or no quantity change) keeps it (the FIX-conventional rule); `QuantityKeepsPriorityModifyPolicy` says only a price change loses priority, so a quantity increase keeps its place in the queue. The engine's `modifyPolicy` is never null — the constructor parameter defaults to `nullptr` and a `nullptr` is substituted with a shared, stateless `DefaultModifyPolicy`, so existing call sites that predate the parameter (including `MatchingEngineFacade`) get the conventional rule rather than a null dereference on the first modify. A custom policy must not return `losesPriority = false` for a request that changes the price: `Order` has no price mutator, so the in-place branch applies only `modifyOriginalQty` and would silently drop the new price while returning `RejectionReason::None`. Increase/decrease is measured against the resting order's **total** (`getOriginalQty()`, FIX `OrderQty`), never its remaining `getQty()` — both sides of that comparison must be totals, or a partially-filled order gets misread (with 4 of 10 filled, a modify to a total of 8 is a *decrease* to 4 working, but reads as an increase against the remaining 6). If priority is kept, the resting `Order` is mutated in place via `modifyOriginalQty` and stays at its position. If priority is lost, a fresh `Order` is built as an exact replica of the resting order's quantities — constructed with the resting `getOriginalQty()`, then `reduceQty(getFilledQty())` so it carries the same `getFilledQty()` and remaining `getQty()` — and only then, if `newOriginalQty` is given, `modifyOriginalQty(newOriginalQty)` is applied. This preserves fill history (`CumQty`) across the replacement: building it from the new total alone would reset `getFilledQty()` to zero and lose what already traded. The replacement takes the new price (or the old one if unchanged), the *request's* timestamp (so it loses FIFO priority as intended), and the resting order's side/type/`TimeInForce`/`postOnly`. Because `executeMatching` derives the incoming order's initial quantity from `getOriginalQty()` rather than `getQty()`, a partially filled replacement that rests unchanged ends as `PartiallyExecuted`, not `Pending`. It is then run through the normal matching path — so a repriced modify can execute trades and is subject to the price collar, post-only, and STP exactly like a new order.

**Order of operations in the cancel-replace path matters and is load-bearing.** `matchOrder` is split into `checkBeforeMatching` (post-only, FOK, price collar) and `executeMatching` (the matching loop and resting the remainder); `matchOrder` itself is field validation, the duplicate-ID check, then those two in sequence. The modify path deliberately runs `checkBeforeMatching(replacement)` **before** `LimitOrderBook::cancelOrder` removes the original, then calls `executeMatching` directly rather than re-entering `matchOrder`. Doing the removal first — as an earlier version did — breaks two things at once: a rejected modify destroys the order (it has already left the book and nothing puts it back, so the caller is told "rejected" while their order silently vanishes), and pulling the order out moves the collar's own reference price, since `violatesPriceCollar` falls back to `getMidPrice()`, which returns `std::nullopt` the moment one side empties — so removing the only bid makes the collar skip the check entirely and a wildly-priced replacement executes. Checking first means a rejection is a true no-op: the order keeps its place in the queue and the book is untouched. `executeMatching` is called directly because the replacement reuses the original's `OrderID`, so `matchOrder`'s duplicate-ID check would be meaningless here, and re-running the collar after the removal could diverge from the check that already passed. The reference price is therefore evaluated against the book as it stood when the request arrived, including the order being withdrawn. Note `TimeInForce::FOK`/`IOC` orders never rest (`executeMatching` discards any remainder), so a modify only ever targets a GTC order and the FOK branch is unreachable from this path.

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
│   ├── models/         # Order, Trade, MarketStructureSnapshot, RejectionReason, enums (Side, OrderType, TimeInForce, OrderStatus), IRequest hierarchy (NewOrderRequest, CancelRequest, ModifyRequest)
│   ├── engine/         # LimitOrderBook, MatchingEngine, ExecutionEngine, book_side_ops
│   ├── policy/         # OrderValidator, OrderLifecycle, STPPolicy, ModifyPolicy
│   ├── infra/          # TradeLogger, TradeIdGenerator (interfaces + concrete impls)
│   └── utils/          # order_utils.hpp (isSelfTrade helper), constants.hpp (tick/lot/time defaults)
├── src/                # .cpp implementations mirroring include/
├── test/               # GoogleTest files mirroring include/models, include/policy, include/infra
└── python_binding/     # MatchingEngineFacade + pybind11 bindings
googletest/             # git submodule
build/                  # CMake out-of-tree build (do not edit)
```

The Python API is fully documented in `PYTHON_LIBRARY.md`, including all enum values, `Order` validation rules, `MatchingEngine` constructor string options, and the `snapshot()` return dict schema.
