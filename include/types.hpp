#pragma once
// Represents a request to buy/sell
struct Order {
    enum class Side {
        Buy,
        Sell
    };

    enum class Type {
        Market, // execute at any price
        Limit // execute at this price or better
    };

    enum class TimeInForce {
        GTC, // Good Till Cancelled — default, rests on book until filled or cancelled
        IOC, // Immediate or Cancel — fill what you can, cancel remainder
        FOK // Fill or Kill — fill everything or cancel entirely
    };

    int id;
    int seqNum;
    int price;
    int quantity;

    Side side;
    Type type;
    TimeInForce tif;

    Order* prev { nullptr };
    Order* next { nullptr };
};


// Represents a transaction
struct Trade {
    int buyOrderId;
    int sellOrderId;

    int price;
    int quantity;
};