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

### OrderType
- LIMIT
- MARKET

### RequestType
- NEW
- CANCEL
- MODIFY

### TimeInForce
- GTC (Good-Til-Cancelled) — default. Any unfilled remainder rests in the book.
- IOC (Immediate-or-Cancel) — matches as much as immediately available; any
  unfilled remainder is discarded instead of resting.
- FOK (Fill-or-Kill) — all-or-nothing. If the full quantity cannot be matched
  immediately, the order is cancelled with zero fills.

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
- DUPLICATE_ORDER_ID
- ORDER_TO_BE_CANCELLED_DOES_NOT_EXIST
- ORDER_BOOK_INVARIANT_VIOLATION
- SELF_TRADE_PREVENTION
- FOK_INSUFFICIENT_LIQUIDITY
- INVALID_POST_ONLY_ORDER
- POST_ONLY_WOULD_CROSS
- PRICE_COLLAR_VIOLATION
- INVALID_MODIFY_ORDER
- ORDER_TO_BE_MODIFIED_DOES_NOT_EXIST
- MODIFY_QUANTITY_BELOW_FILLED
- NO_OP_MODIFY

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
    time_in_force: lob.TimeInForce = lob.TimeInForce.GTC,
    post_only: bool = False,
)
```

### Read-only properties

- `order_id`
- `owner_id`
- `price_ticks`
- `qty`
- `original_qty`
- `filled_qty`
- `side`
- `order_type`
- `timestamp`
- `status`
- `time_in_force`
- `post_only`

### Methods

- `is_cancelled() -> bool`
- `is_executed() -> bool`

### Ownership across the Python/C++ boundary

- An `Order` you construct in Python and the corresponding `Order` resting in the book (or held anywhere in C++) are the **same object**, not a copy. `Order` is bound with a `shared_ptr` holder, so passing an `Order` into `match_order` shares ownership between your Python variable and the engine rather than cloning it.
- Because of that, **your existing Python `Order` object is mutated in place** as the engine processes it — `status` and `qty` change on the reference you already hold. You never need to re-fetch it or call `match_order` again to observe a fill, a cancellation, or an STP outcome.
- Python cannot mutate an `Order` directly: every property is read-only, and the engine's internal mutators are intentionally not exposed to Python. The only way to change an order's `qty`/`status` is to submit it through `match_order`, or to cancel it via a `CancelRequest` submitted through `cancel()`.
- A resting order outlives your Python reference. If you let your only variable for a resting order go out of scope (or `del` it), the underlying `Order` is kept alive by the book's own `shared_ptr` — it is not destroyed. There is currently **no API to fetch a handle to a resting order by `order_id`**; `snapshot()` only returns aggregated price-level data, not individual orders. In practice, keep a reference to any `Order` you want to inspect or cancel later.
- Constructing a second `Order` with the same `order_id` as one already resting in the book does **not** give you a handle to that resting order — it's a distinct object. Submitting it is rejected with `RejectionReason.DUPLICATE_ORDER_ID` and never merges with or mutates the original.
- A `CancelRequest` is not an `Order` — it's a separate, immutable message whose `target_order_id` tells the engine which resting order to remove. It is never stored in the book and is never mutated by the engine. On success, `cancel()` returns a `SubmitResult` with `reason == RejectionReason.NONE` and `order` set to the *targeted* order, whose `status`/`qty` actually change. If the cancel is rejected, the resting order is untouched.
- A `ModifyRequest` follows the same rule: it is an immutable message and is never stored in the book. It changes the price and/or the total quantity of a resting order.
- **After a modify, rebind your variable to `result.order`.** A modify that loses queue priority (a price change, or a quantity increase under the default policy) cancels the resting order and puts a *replacement* in the book with the same `order_id`. The `Order` object you held before is then cancelled and stale. The replacement is `result.order`. A modify that keeps priority changes the resting order in place, and `result.order` is that same object. A rejected modify returns the original, still resting, so `order = engine.modify(request).order or order` is always safe — see [Class: SubmitResult](#class-submitresult).

### Order validation rules (important)

- LIMIT orders require:
  - `price_ticks > 0`
  - `qty > 0`
  - `side` is BUY or SELL
  - `order_id != 0`
  - if `post_only` is set, `time_in_force == TimeInForce.GTC` (post-only
    guarantees maker status, which IOC/FOK's semantics contradict)
- MARKET orders require:
  - `price_ticks == 0`
  - `qty > 0`
  - `side` is BUY or SELL
  - `order_id != 0`
  - `post_only == False` (a market order is always marketable by
    definition, so post-only on it is meaningless; violating this rule is
    reported as `RejectionReason.INVALID_MARKET_ORDER`, not
    `INVALID_POST_ONLY_ORDER`)

Invalid orders are rejected through `RejectionReason`. `price_ticks` and `qty`
map to signed 32-bit C++ integers, so a negative value stays negative across the
binding and is rejected by the rules above rather than wrapping to a large
positive number.

`CancelRequest.validate()` requires `target_order_id != 0`, reported as
`RejectionReason.INVALID_CANCEL_ORDER` — see [Class: CancelRequest](#class-cancelrequest).

`ModifyRequest.validate()` is described under [Class: ModifyRequest](#class-modifyrequest).

### Time-in-force semantics

- `time_in_force` applies to LIMIT and MARKET orders. MARKET orders already
  discard any unfilled remainder, so IOC has no additional effect on them
  beyond FOK's all-or-nothing check.
- FOK is evaluated as a read-only pass over the book before any fills happen:
  it walks the marketable price levels on the opposing side and sums
  available quantity. If a same-owner (self-trade) resting order is
  encountered anywhere in that marketable range, the FOK check aborts
  immediately and treats the order as unfillable — it does not skip past
  the same owner resting order and keep counting deeper levels.
- If FOK determines the order is unfillable, the order is cancelled with
  zero fills; the book is left completely unchanged. `match_order` returns
  `RejectionReason.FOK_INSUFFICIENT_LIQUIDITY` in this case, and the order
  ends up with `status == CANCELLED`.

### Post-only semantics

- `post_only` only applies to LIMIT orders with `time_in_force == GTC` —
  see the LIMIT/MARKET validation rules above. `RejectionReason.INVALID_POST_ONLY_ORDER`
  is only returned for a LIMIT order combining `post_only` with a non-GTC
  `time_in_force`; a `post_only` MARKET order is rejected as
  `INVALID_MARKET_ORDER` instead.
- The check happens before any matching is attempted: if the incoming order
  would immediately cross the book (i.e. it is marketable), it is rejected
  with `RejectionReason.POST_ONLY_WOULD_CROSS` and `status == CANCELLED`.
  The book is left completely unchanged — no partial fills ever occur for a
  post-only order.
- The would-cross check runs before self-trade prevention, so a resting
  order from the same owner at a crossable price still causes rejection
  (it is never reached to be cancelled by STP).
- If the order would not cross, it rests in the book exactly like a normal
  GTC LIMIT order.

### Price collar semantics

- Enabled by passing `max_deviation_ticks` to the `MatchingEngine` constructor
  (see below). It is `None` by default, which disables the check entirely.
- Only applies to LIMIT orders — MARKET orders always bypass the collar, since
  they have no limit price to validate against.
- The reference price is the price of the most recent trade if any trade has
  occurred; otherwise it falls back to the current book mid
  (`(best_bid + best_ask) / 2`). If neither is available (e.g. an empty or
  one-sided book with no trades yet), the check is skipped and the order is
  accepted.
- A LIMIT order priced outside `[reference - max_deviation_ticks, reference +
  max_deviation_ticks]` is rejected with
  `RejectionReason.PRICE_COLLAR_VIOLATION` and `status == CANCELLED`. Orders at
  exactly the boundary are accepted.
- This check runs after the Post-Only and FOK-insufficient-liquidity checks,
  so those two rejections take precedence over a price collar violation.

---

## Class: CancelRequest

`CancelRequest` is an immutable inbound message, distinct from `Order` — it is
never stored in the book and never mutated by the engine.

### Constructor

```python
lob.CancelRequest(
    request_id: int,
    owner_id: int,
    timestamp: int,
    target_order_id: int,
)
```

### Read-only properties

- `request_id`
- `request_type` (always `lob.RequestType.CANCEL`)
- `owner_id`
- `timestamp`
- `target_order_id`

### Methods

- `validate() -> lob.RejectionReason` — field-level validation only
  (`target_order_id != 0`). Book-level checks (existence, ownership) happen
  inside `MatchingEngine.cancel()`.

---

## Class: ModifyRequest

`ModifyRequest` is an immutable inbound message that changes a resting order
with a FIX-style cancel-replace. It is never stored in the book.

### Constructor

```python
lob.ModifyRequest(
    request_id: int,
    owner_id: int,
    timestamp: int,
    target_order_id: int,
    new_price_ticks: Optional[int] = None,
    new_original_qty: Optional[int] = None,
)
```

`new_original_qty` is the new **total** quantity of the order (FIX `OrderQty`),
not the new remaining quantity. For an order with `original_qty == 10` and
`filled_qty == 4`, `new_original_qty=8` leaves 4 working. Compare it with
`Order.original_qty` and `Order.filled_qty`, never with `Order.qty`.

### Read-only properties

- `request_id`
- `request_type` (always `lob.RequestType.MODIFY`)
- `owner_id`
- `timestamp`
- `target_order_id`
- `new_price_ticks` — `None` if not given
- `new_original_qty` — `None` if not given

### Methods

- `validate() -> lob.RejectionReason` — field-level validation only.
  Returns `INVALID_MODIFY_ORDER` if `target_order_id == 0`, or if a given
  `new_price_ticks` or `new_original_qty` is not positive. Returns
  `NO_OP_MODIFY` if neither field is given.

### How the engine handles a modify

Checks run in this order:

1. `validate()` fails: the reason from `validate()`.
2. The order does not exist, or belongs to a different `owner_id`:
   `ORDER_TO_BE_MODIFIED_DOES_NOT_EXIST` (the same reason for both, so a
   caller cannot probe for another owner's order).
3. The given price and quantity both equal the current ones: `NO_OP_MODIFY`.
   The order keeps its place in the queue.
4. `new_original_qty < filled_qty`: `MODIFY_QUANTITY_BELOW_FILLED`.
5. `new_original_qty == filled_qty`: this is "done for day". The remainder is
   cancelled and the reason is `NONE`.
6. Otherwise the `modify_policy` decides whether the order loses its queue
   priority.

If the order keeps its priority, the engine changes it in place. If it loses
priority, the engine builds a replacement with the same `order_id`, the
request's `timestamp`, and the same fill history, and runs it through normal
matching. A repriced modify can therefore **execute trades**, and it is subject
to the price collar, post-only and self-trade prevention like a new order.

A modify rejected by the price collar or post-only check leaves the original
order in the book, in its original queue position.

---

## Class: SubmitResult

`cancel()` and `modify()` return a `SubmitResult`. You do not construct it.

### Read-only properties

- `reason: lob.RejectionReason`
- `order: Optional[lob.Order]`

`order` is the order you should look at next. It is `None` when validation
fails, and when the order does not exist or belongs to a different owner. The
same `(reason, None)` result is returned for both of those cases, so a caller
cannot tell another owner's order from an order that never existed.

| Call | Outcome | `order` |
|---|---|---|
| `cancel` | success | the cancelled order (no longer in the book) |
| `modify` | priority kept | the resting order, changed in place |
| `modify` | priority lost, accepted | the replacement (it may already be filled, or cancelled by STP) |
| `modify` | done for day | the cancelled resting order |
| `modify` | `NO_OP_MODIFY`, `MODIFY_QUANTITY_BELOW_FILLED` | the resting order, unchanged |
| `modify` | rejected by price collar or post-only | the original resting order, unchanged |

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
    max_deviation_ticks: Optional[int] = None,
    modify_policy: str = "mp1",
)
```

`max_deviation_ticks` enables the price collar (see "Price collar semantics"
above) when set; `None` (the default) disables it. It is in raw ticks, not
scaled by `tick_size`.

### Constructor options

All string options below are case-insensitive (for example, `"CANCEL_BOTH"` and `"cancel_both"` are equivalent).

- `stp_policy` *(required)*:
  - `"cancel_both"`
  - `"cancel_incoming"`
  - `"cancel_resting"`

- `modify_policy` *(default `"mp1"`)* — decides which modifies lose queue priority:
  - `"mp1"` — a price change or a quantity increase loses priority. A
    quantity decrease keeps it. This is the usual exchange rule.
  - `"mp2"` — only a price change loses priority. A quantity increase keeps
    the order's place in the queue.

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
- The same `Order` object is mutated in-place by the engine (status/qty updates as matching proceeds) — see [Ownership across the Python/C++ boundary](#ownership-across-the-pythonc-boundary).

#### cancel(request: lob.CancelRequest) -> lob.SubmitResult

Submits a cancel request for a resting order. **Breaking change:** this
returned a bare `RejectionReason` before. Use `result.reason` where you
compared the return value.

- `result.reason` is `RejectionReason.NONE` on success; the targeted resting
  order's `status` becomes `CANCELLED` (or `CANCELLED_AFTER_PARTIAL_EXECUTION`
  if it had already partially filled), and it is removed from the book.
  `result.order` is that order.
- `result.reason` is `RejectionReason.INVALID_CANCEL_ORDER` if
  `request.validate()` fails.
- `result.reason` is `RejectionReason.ORDER_TO_BE_CANCELLED_DOES_NOT_EXIST` if
  `target_order_id` doesn't exist in the book, or exists but belongs to a
  different `owner_id` than `request.owner_id` (the same reason is returned
  in both cases, deliberately, so a caller cannot probe for the existence of
  another owner's order).
- `result.order` is `None` on every rejection.

#### modify(request: lob.ModifyRequest) -> lob.SubmitResult

Submits a cancel-replace modify for a resting order. See
[Class: ModifyRequest](#class-modifyrequest) for the checks and their order,
and [Class: SubmitResult](#class-submitresult) for what `result.order` holds in
each case.

- `result.reason` is `RejectionReason.NONE` on success.
- Other reasons: `INVALID_MODIFY_ORDER`, `ORDER_TO_BE_MODIFIED_DOES_NOT_EXIST`,
  `NO_OP_MODIFY`, `MODIFY_QUANTITY_BELOW_FILLED`, and, for a modify that loses
  priority and goes back through matching, `PRICE_COLLAR_VIOLATION`,
  `POST_ONLY_WOULD_CROSS` and `SELF_TRADE_PREVENTION`.
- After a modify that loses priority, **rebind your `Order` variable to
  `result.order`**. The object you held is cancelled.

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
    "total_order_count": int,
    "total_notional_value": float,
  },
  "ask_summary": {
    "total_quantity": float,
    "total_order_count": int,
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

cancel_request = lob.CancelRequest(
    request_id=1,
    owner_id=101,
    timestamp=2,
    target_order_id=1,
)
engine.cancel(cancel_request)
```

Modify, then cancel the replacement:

```python
limit_buy = lob.Order(
    order_id=2,
    owner_id=101,
    price_ticks=1000,
    qty=10,
    side=lob.Side.BUY,
    order_type=lob.OrderType.LIMIT,
    timestamp=3,
)
engine.match_order(limit_buy)

modify_request = lob.ModifyRequest(
    request_id=2,
    owner_id=101,
    timestamp=4,
    target_order_id=2,
    new_price_ticks=1005,
)
result = engine.modify(modify_request)
assert result.reason == lob.RejectionReason.NONE

assert limit_buy.is_cancelled()
limit_buy = result.order
assert limit_buy.price_ticks == 1005

engine.cancel(lob.CancelRequest(request_id=3, owner_id=101, timestamp=5, target_order_id=2))
```
