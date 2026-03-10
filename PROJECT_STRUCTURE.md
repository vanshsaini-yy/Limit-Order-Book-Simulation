# Project Structure

## Overview
This repository contains a C++23 limit order book simulator with a focus on core matching logic and policy enforcement. The build is driven by CMake with GoogleTest-based unit tests.

## Build & Test Flow
- Entry script: [run.sh](run.sh) (configures, builds, runs tests).
- Top-level build config: [CMakeLists.txt](CMakeLists.txt).
- C++ library config: [cpp/CMakeLists.txt](cpp/CMakeLists.txt) (header-only `lob_core` interface target + tests).

## Directory Map
- [cpp/include](cpp/include)
  - [cpp/include/agents](cpp/include/agents): agent-facing adapters and connectors (empty placeholder today).
  - [cpp/include/data](cpp/include/data): shared runtime constants and configuration helpers.
  - [cpp/include/infra](cpp/include/infra): logging and trade identifier infrastructure.
  - [cpp/include/models](cpp/include/models): core domain models and matching logic.
  - [cpp/include/policy](cpp/include/policy): validation, lifecycle, and self-trade prevention policies.
  - [cpp/include/utils](cpp/include/utils): lightweight helpers.
- [cpp/test](cpp/test)
  - [cpp/test/infra](cpp/test/infra): infrastructure-focused unit tests (loggers, trade ID generators).
  - [cpp/test/models](cpp/test/models): model and engine unit tests.
  - [cpp/test/utils](cpp/test/utils): utility unit tests.
- [googletest](googletest): vendored GoogleTest framework.
- [build](build): generated build artifacts (safe to ignore in reviews).

## Core Headers & Responsibilities
+ [cpp/include/data/constants.hpp](cpp/include/data/constants.hpp)
+  - shared runtime constants used across the simulator (pricing, quantities, etc.).
+ [cpp/include/models/order.hpp](cpp/include/models/order.hpp)
+  - `Order` type and core enums (`Side`, `OrderType`, `OrderStatus`) plus scalar aliases.
+ [cpp/include/models/trade.hpp](cpp/include/models/trade.hpp)
+  - `Trade` type emitted on execution, with trade identifiers and execution details.
+ [cpp/include/models/market_structure_snapshot.hpp](cpp/include/models/market_structure_snapshot.hpp)
+  - `MarketStructureSnapshot` captures best prices, depth, and tempo metrics exposed to agents.
+ [cpp/include/models/order_book.hpp](cpp/include/models/order_book.hpp)
+  - `LimitOrderBook` with bid/ask structures, order tracking, add/remove, and matching helpers.
+ [cpp/include/models/execution_engine.hpp](cpp/include/models/execution_engine.hpp)
+  - `ExecutionEngine` for quantity matching between taker and maker.
+ [cpp/include/models/matching_engine.hpp](cpp/include/models/matching_engine.hpp)
+  - `MatchingEngine` orchestrates matching, self-trade checks, lifecycle updates, and book updates.
+ [cpp/include/infra/trade_logger.hpp](cpp/include/infra/trade_logger.hpp)
+  - `TradeLogger` interface for trade logging sinks.
+ [cpp/include/infra/monotonic_trade_id_generator.hpp](cpp/include/infra/monotonic_trade_id_generator.hpp)
+  - deterministic trade ID generator used during matching.
+ [cpp/include/infra/trade_id_generator.hpp](cpp/include/infra/trade_id_generator.hpp)
+  - abstract trade ID producer used by loggers and engines.
+ [cpp/include/infra/binary_trade_logger.hpp](cpp/include/infra/binary_trade_logger.hpp)
+  - `BinaryTradeLogger` and `TradeLogRecord` for binary logging.
+ [cpp/include/utils/order_utils.hpp](cpp/include/utils/order_utils.hpp)
+  - `isSelfTrade()` helper for ownership checks.

## Policy Components
- [cpp/include/policy/order_validation.hpp](cpp/include/policy/order_validation.hpp)
  - `RejectionReason`: enumerates validation failures used by the book and policies.
  - `OrderValidator`: pre-add and pre-remove validation logic (null, quantity/price, duplicates, cancelled/executed).
- [cpp/include/policy/order_lifecycle.hpp](cpp/include/policy/order_lifecycle.hpp)
  - `OrderLifecycle`: transitions order status after matching or cancellation.
- [cpp/include/policy/self_trade_prevention.hpp](cpp/include/policy/self_trade_prevention.hpp)
  - `STPPolicy`: abstract policy interface for self-trade handling.
  - `CancelBothSTP`: cancels both incoming and resting orders.
  - `CancelIncomingSTP`: cancels only the incoming order.
  - `CancelRestingSTP`: cancels only the resting order.

## Tests Summary
- [cpp/test/models/test_order.cpp](cpp/test/models/test_order.cpp)
- [cpp/test/models/test_trade.cpp](cpp/test/models/test_trade.cpp)
- [cpp/test/models/test_order_book.cpp](cpp/test/models/test_order_book.cpp)
- [cpp/test/models/test_execution_engine.cpp](cpp/test/models/test_execution_engine.cpp)
- [cpp/test/models/test_matching_engine_match.cpp](cpp/test/models/test_matching_engine_match.cpp)
- [cpp/test/models/test_matching_engine_stp.cpp](cpp/test/models/test_matching_engine_stp.cpp)
- [cpp/test/utils/test_order_utils.cpp](cpp/test/utils/test_order_utils.cpp)
- [cpp/test/infra/test_binary_trade_logger.cpp](cpp/test/infra/test_binary_trade_logger.cpp)
- [cpp/test/infra/test_trade_id_generator.cpp](cpp/test/infra/test_trade_id_generator.cpp)

## Notes on Build Artifacts
- [build](build) contains generated CMake and test outputs; it is not source and can be deleted and regenerated safely.
