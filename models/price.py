class Price:
    __slots__ = ("ticks",)

    def __init__(self, value, tick_size):
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