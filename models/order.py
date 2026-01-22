from dataclasses import dataclass

@dataclass(slots=True)
class Order:
    order_id: int       # unique identifier for the order
    side: int           # 1 for buy, -1 for sell
    price: int          # price in ticks
    quantity: int       # quantity in lots
    order_type: int     # 0 for limit order, 1 for market order, 2 for cancel order
    timestamp: int      # time when the order was placed