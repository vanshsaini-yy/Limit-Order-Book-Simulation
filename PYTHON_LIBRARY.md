# Python Library Guide

This document describes the user-facing Python API exposed by the C++ limit order book engine via pybind11.

## Module

- Module name: `limit_order_book`
- Built from C++ sources under `cpp/python_binding/`

## Build and import

From the repository root:

```bash
bash run.sh -DLOB_BUILD_PYTHON_BINDINGS=ON
```

For local scripts, the built extension is typically available under `build/cpp/`.

```python
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "build" / "cpp"))

import limit_order_book as lob
```

---

## Enums

### Side
- BUY
- SELL
- NONE

### OrderType
- LIMIT
- MARKET
- CANCEL

### OrderStatus
- PENDING
- PARTIALLY_EXECUTED
- EXECUTED
- CANCELLED
- CANCELLED_AFTER_PARTIAL_EXECUTION

### RejectionReason
- NONE
- NULL_ORDER
- INVALID_ORDER_TYPE
- INVALID_LIMIT_ORDER
- INVALID_MARKET_ORDER
- INVALID_CANCEL_ORDER
- ORDER_TO_BE_ADDED_ALREADY_EXISTS
- ORDER_TO_BE_CANCELLED_DOES_NOT_EXIST
- ORDER_BOOK_INVARIANT_VIOLATION

---

## Class: Order

### Constructor

```python
lob.Order(
    order_id: int,
    owner_id: int,
    price_ticks: int,
    qty: int,
    side: lob.Side,
    order_type: lob.OrderType,
    timestamp: int,
    linked_order_id: int = 0,
)
```

### Read-only properties

- `order_id`
- `owner_id`
- `price_ticks`
- `qty`
- `side`
- `order_type`
- `timestamp`
- `status`
- `linked_order_id`

### Methods

- `is_cancelled() -> bool`
- `is_executed() -> bool`

### Order validation rules (important)

- LIMIT orders require:
  - `price_ticks > 0`
  - `qty > 0`
  - `side` is BUY or SELL
  - `order_id != 0`
  - `linked_order_id == 0`
- MARKET orders require:
  - `price_ticks == 0`
  - `qty > 0`
  - `side` is BUY or SELL
  - `order_id != 0`
  - `linked_order_id == 0`
- CANCEL orders require:
  - `price_ticks == 0`
  - `qty == 0`
  - `side` is NONE
  - `order_id != 0`
  - `linked_order_id != 0`
  - `linked_order_id != order_id`

Invalid orders are rejected through `RejectionReason`.

---

## Class: MatchingEngine

### Constructor

```python
lob.MatchingEngine(
    stp_policy: str,
    trade_id_generator: str = "none",
    trade_id_start: int = 1,
    trade_logger: str = "none",
    trade_log_file_path: str = "trades.bin",
    tick_size: float = 0.01,
    lot_size: float = 1.0,
    time_interval: float = 1.0,
)
```

### Constructor options

All string options below are case-insensitive (for example, `"CANCEL_BOTH"` and `"cancel_both"` are equivalent).

- `stp_policy` *(required)*:
  - `"cancel_both"`
  - `"cancel_incoming"`
  - `"cancel_resting"`

- `trade_id_generator`:
  - `"none"`
  - `"monotonic"`

- `trade_logger`:
  - `"none"`
  - `"binary"`

`trade_log_file_path` is used when `trade_logger == "binary"`.

> If `trade_logger != "none"`, a trade ID generator is required (`trade_id_generator` cannot be `"none"`).

Invalid option values raise `ValueError` from Python (mapped from C++ `std::invalid_argument`).

### Read-only properties

- `tick_size: float`
- `lot_size: float`
- `time_interval: float`

### Scaling

All input to the engine uses raw integer units. The three scalars are applied only at output (in `snapshot()`):

| Scalar | Applied to |
|---|---|
| `tick_size` | `best_bid`, `best_ask`, `spread`, `mid`, depth `price`, `total_notional_value` |
| `lot_size` | `total_quantity` (summary + depth), `total_volume_traded`, `total_notional_value` |
| `time_interval` | `timestamp` |

`total_notional_value` is computed as `sum(price_ticks × qty)` internally and then scaled by `tick_size × lot_size`, giving true currency notional.

`Order.price_ticks` (input) is always a raw integer. `snapshot()` depth `price` is always a scaled float. Never pass a scaled float where `price_ticks` is expected.

### Methods

#### match_order(order: lob.Order) -> lob.RejectionReason

Submits an order for validation/matching/cancellation.

- Returns `RejectionReason.NONE` on success.
- Returns a non-`NONE` rejection reason when not accepted.
- The same `Order` object is mutated in-place by the engine (status/qty updates as matching proceeds).

#### snapshot(now: int, depth_limit: int = 5) -> dict

Returns a dictionary describing market structure:

```python
{
  "timestamp": float,
  "best_bid": Optional[float],
  "best_ask": Optional[float],
  "spread": Optional[float],
  "mid": Optional[float],
  "bid_summary": {
    "total_quantity": float,
    "order_count": int,
    "total_notional_value": float,
  },
  "ask_summary": {
    "total_quantity": float,
    "order_count": int,
    "total_notional_value": float,
  },
  "bid_depths": [
    {"price": float, "total_quantity": float, "order_count": int},
    ...
  ],
  "ask_depths": [
    {"price": float, "total_quantity": float, "order_count": int},
    ...
  ],
  "tempo": {
    "trade_execution_count": int,
    "order_cancellation_count": int,
    "total_volume_traded": float,
  },
}
```

---

## Minimal usage example

```python
import limit_order_book as lob


engine = lob.MatchingEngine(
    stp_policy="cancel_both",
    trade_id_generator="none",
    trade_logger="none",
)

limit_buy = lob.Order(
    order_id=1,
    owner_id=101,
    price_ticks=1000,
    qty=10,
    side=lob.Side.BUY,
    order_type=lob.OrderType.LIMIT,
    timestamp=1,
)

result = engine.match_order(limit_buy)
assert result == lob.RejectionReason.NONE

snap = engine.snapshot(now=1, depth_limit=5)
print(snap["best_bid"], snap["best_ask"], snap["mid"])

cancel = lob.Order(
    order_id=2,
    owner_id=101,
    price_ticks=0,
    qty=0,
    side=lob.Side.NONE,
    order_type=lob.OrderType.CANCEL,
    timestamp=2,
    linked_order_id=1,
)
engine.match_order(cancel)
```
