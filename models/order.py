from models.price import Price
from dataclasses import dataclass

@dataclass(slots=True)
class Order:
    order_id: int
    side: str
    price: Price
    quantity: int
    order_type: str