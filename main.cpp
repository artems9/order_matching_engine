#include <vector>
#include <queue>
#include <iostream>

struct Order 
{
    enum class Type { Buy, Sell };

    int id; 
    int price;
    int quantity;
    Type type;
};


struct CompareBuys
{
    // for buys we want highest prices on top so return true for bigger value
    bool operator() (const Order& a, const Order& b) { return a.price < b.price; }
};

struct CompareSells
{
    // for sells we want the lowest prices on top so return true for smaller value
    bool operator() (const Order& a, const Order& b) { return a.price > b.price; }
};


struct OrderBook
{
    // max heap for buy orders since highest bidder wins
    std::priority_queue<Order, std::vector<Order>, CompareBuys> buyOrders;
    // min heap for sell orders since lowest seller wins
    std::priority_queue<Order, std::vector<Order>, CompareSells> sellOrders;

    void addOrder(const Order& order)
    {
        // buy order
        if (order.type == Order::Type::Buy) { buyOrders.push(order); }
        // sell order
        else { sellOrders.push(order); }
    }

    bool tryMatchingOrders()
    {
        // make sure have buy and sell orders available
        if (buyOrders.empty() || sellOrders.empty()) { return false; }

        Order buyOrder = buyOrders.top();
        Order sellOrder = sellOrders.top();
        // no match found
        if (buyOrder.price < sellOrder.price) { return false; }

        buyOrders.pop();
        sellOrders.pop();

        // make trade
        int quantityTraded = std::min(sellOrder.quantity, buyOrder.quantity);
        buyOrder.quantity -= quantityTraded;
        sellOrder.quantity -= quantityTraded;
        std::cout << "Matched " << quantityTraded

                      << " @ " << sellOrder.price << std::flush;
        // put whats left back into orderbook
        if (buyOrder.quantity > 0) { buyOrders.push(buyOrder); }
        if (sellOrder.quantity > 0) { sellOrders.push(sellOrder); }
        return true;
    }
};


int main() 
{
    Order o1 {0, 10, 100, Order::Type::Buy};
    Order o3 {2, 15, 100, Order::Type::Buy};
    Order o6 {2, 20, 100, Order::Type::Buy};

    Order o2 {1, 10, 100, Order::Type::Sell};
    Order o4 {3, 18, 100, Order::Type::Sell};
    Order o5 {4, 15, 100, Order::Type::Sell};

    OrderBook ob;
    ob.addOrder(o1);
    ob.addOrder(o2);
    ob.addOrder(o3);
    ob.addOrder(o4);
    ob.addOrder(o5);
    ob.addOrder(o6);


    while (ob.tryMatchingOrders()) {};

    return 0;
};
