#include "MatchingEngine.hpp"
#include "Types.hpp"
#include <iostream>
#include <sstream>

int main() {
    MatchingEngine engine;
    std::string line;
    int id{0};
    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        std::string side;
        int price;
        int qty;
        ss >> side >> price >> qty;
        Order::Side pside = (side == "BUY") ? Order::Side::Buy : Order::Side::Sell;
        Order order{id, id, price, qty, pside, Order::Type::Limit, Order::TimeInForce::GTC};
        ++id;
        std::vector<Trade> trades = engine.matchIncomingOrder(order);
        for (const auto& trade : trades) {
            std::cout << "traded : " << trade.quantity << "BTC\n";
            std::cout << "at : " << trade.price << "$\n";
        }
    }
    return 0;
}