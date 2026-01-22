from constants import TICK_SIZE
class Price:
    __slots__ = ("ticks",)

    def __init__(self, value, tick_size=TICK_SIZE):
        self.ticks = int(round(value / tick_size))

    def __lt__(self, other):
        return self.ticks < other.ticks

    def __le__(self, other):
        return self.ticks <= other.ticks

    def __eq__(self, other):
        return self.ticks == other.ticks

    def __hash__(self):
        return hash(self.ticks)

    def to_float(self, tick_size):
        return self.ticks * tick_size