class Order {
    private:
        int orderId;
        int side;
        int ticks;
        int quantity;
        int timeStamp;
        int orderType;

    public:
        Order(int orderId, int side, int ticks, int quantity, int timeStamp, int orderType)
            : orderId(orderId), side(side), ticks(ticks), quantity(quantity), timeStamp(timeStamp), orderType(orderType) {}
};