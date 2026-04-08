# Project Structure

## Overview
This repository contains a C++23 limit order book simulator with a focus on core matching logic and policy enforcement. The build is driven by CMake with GoogleTest-based unit tests.

## Build & Test Flow
- Entry script: [run.sh](run.sh) (creates/uses [build](build), runs configure, build, then `ctest --output-on-failure` from inside [build](build)).
- Top-level build config: [CMakeLists.txt](CMakeLists.txt).
- C++ library config: [cpp/CMakeLists.txt](cpp/CMakeLists.txt) (header-only `lob_core` interface target, tests, and optional `pybind11`-based Python bindings when `LOB_BUILD_PYTHON_BINDINGS=ON`).

## Directory Map
- [cpp/include](cpp/include)
  - [cpp/include/infra](cpp/include/infra): logging and trade identifier infrastructure.
  - [cpp/include/models](cpp/include/models): core domain models and matching logic.
  - [cpp/include/policy](cpp/include/policy): validation, lifecycle, and self-trade prevention policies.
  - [cpp/include/utils](cpp/include/utils): lightweight helpers.
- [cpp/python_binding](cpp/python_binding): binding layer that exposes selected C++ models and matching-engine workflows to Python via `pybind11`.
- [cpp/test](cpp/test)
  - [cpp/test/infra](cpp/test/infra): infrastructure-focused unit tests (loggers, trade ID generators).
  - [cpp/test/models](cpp/test/models): model and engine unit tests.
  - [cpp/test/utils](cpp/test/utils): utility unit tests.
- [googletest](googletest): GoogleTest framework (vendored if present locally; otherwise fetched via `FetchContent` during CMake configure).
- [build](build): generated build artifacts (safe to ignore in reviews).

## Core Headers & Responsibilities
- [cpp/include/models/order.hpp](cpp/include/models/order.hpp)
  - `Order` type and core enums (`Side`, `OrderType`, `OrderStatus`) plus scalar aliases.
- [cpp/include/models/trade.hpp](cpp/include/models/trade.hpp)
  - `Trade` type emitted on execution, with trade identifiers and execution details.
- [cpp/include/models/market_structure_snapshot.hpp](cpp/include/models/market_structure_snapshot.hpp)
  - `MarketStructureSnapshot` captures best prices, depth, and tempo metrics exposed to agents.
- [cpp/include/models/order_book.hpp](cpp/include/models/order_book.hpp)
  - `LimitOrderBook` with bid/ask structures, order tracking, add/remove, and matching helpers.
- [cpp/include/models/execution_engine.hpp](cpp/include/models/execution_engine.hpp)
  - `ExecutionEngine` for quantity matching between taker and maker.
- [cpp/include/models/matching_engine.hpp](cpp/include/models/matching_engine.hpp)
  - `MatchingEngine` orchestrates matching, self-trade checks, lifecycle updates, and book updates.
- [cpp/include/infra/trade_logger.hpp](cpp/include/infra/trade_logger.hpp)
  - `TradeLogger` interface for trade logging sinks.
- [cpp/include/infra/monotonic_trade_id_generator.hpp](cpp/include/infra/monotonic_trade_id_generator.hpp)
  - deterministic trade ID generator used during matching.
- [cpp/include/infra/trade_id_generator.hpp](cpp/include/infra/trade_id_generator.hpp)
  - abstract trade ID producer used by loggers and engines.
- [cpp/include/infra/binary_trade_logger.hpp](cpp/include/infra/binary_trade_logger.hpp)
  - `BinaryTradeLogger` and `TradeLogRecord` for binary logging.
- [cpp/include/utils/order_utils.hpp](cpp/include/utils/order_utils.hpp)
  - `isSelfTrade()` helper for ownership checks.

## Python Binding Components
- [cpp/python_binding/python_module.cpp](cpp/python_binding/python_module.cpp)
  - module entry point that assembles the exported Python module.
- [cpp/python_binding/order_bindings.hpp](cpp/python_binding/order_bindings.hpp)
  - declarations for registering order-related enums and types with `pybind11`.
- [cpp/python_binding/order_bindings.cpp](cpp/python_binding/order_bindings.cpp)
  - bindings for `Order` and related domain enums exposed to Python.
- [cpp/python_binding/matching_engine_facade.hpp](cpp/python_binding/matching_engine_facade.hpp)
  - façade API that presents a Python-friendly wrapper around matching-engine operations.
- [cpp/python_binding/matching_engine_facade.cpp](cpp/python_binding/matching_engine_facade.cpp)
  - façade implementation that adapts Python-facing calls to core engine behavior.
- [cpp/python_binding/matching_engine_facade_bindings.hpp](cpp/python_binding/matching_engine_facade_bindings.hpp)
  - declarations for registering the façade bindings with the Python module.
- [cpp/python_binding/matching_engine_facade_bindings.cpp](cpp/python_binding/matching_engine_facade_bindings.cpp)
  - `pybind11` bindings for the matching-engine façade and Python-visible execution flow.

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
- [cpp/test/models/test_order_book.cpp](cpp/test/models/test_order_book.cpp)
- [cpp/test/models/test_execution_engine.cpp](cpp/test/models/test_execution_engine.cpp)
- [cpp/test/models/test_trade.cpp](cpp/test/models/test_trade.cpp)
- [cpp/test/models/test_matching_engine_validation.cpp](cpp/test/models/test_matching_engine_validation.cpp)
- [cpp/test/models/test_matching_engine_match.cpp](cpp/test/models/test_matching_engine_match.cpp)
- [cpp/test/models/test_matching_engine_stp.cpp](cpp/test/models/test_matching_engine_stp.cpp)
- [cpp/test/models/test_matching_engine_execution.cpp](cpp/test/models/test_matching_engine_execution.cpp)
- [cpp/test/utils/test_order_utils.cpp](cpp/test/utils/test_order_utils.cpp)
- [cpp/test/infra/test_binary_trade_logger.cpp](cpp/test/infra/test_binary_trade_logger.cpp)
- [cpp/test/infra/test_trade_id_generator.cpp](cpp/test/infra/test_trade_id_generator.cpp)

## Notes on Build Artifacts
- [build](build) contains generated CMake and test outputs; it is not source and can be deleted and regenerated safely.
