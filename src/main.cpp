// 1. Corresponding Header
// 2. Project Headers
#include "MatchingEngine.hpp"
#include "ThreadPool.hpp"
#include "Types.hpp"
// 3. Third Party Headers
// 4. Standard Library
#include <iostream>
#include <chrono>
#include <sstream>

int main() {
    MatchingEngine engine;
    std::string line;
    int id {0};
    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        std::string side;
        int price;
        int qty;
        ss >> side >> price >> qty;
        Order::Side pside = (side == "BUY") ? Order::Side::Buy : Order::Side::Sell;
        Order order{id, id, price, qty, pside, Order::Type::Limit, Order::TimeInForce::GTC };
        ++id;
        std::vector<Trade> trades = engine.matchIncomingOrder(order);
        for (const auto& trade : trades) {
            std::cout << "traded : " << trade.quantity << "BTC" << std::endl;
            std::cout << "at : " << trade.price << "$" << std::endl;
        }
    }
    return 0;
}