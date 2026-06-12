// 1. Corresponding Header
// 2. Project Headers
#include "Types.h"
#include "MatchingEngine.h"
#include "ThreadPool.h"
// 3. Third Party Headers
// 4. Standard Library
#include <iostream>
#include <chrono>

int main() {
    std::cout << "starting\n";
    constexpr int NUM_ORDERS = 1000000;
    auto start = std::chrono::high_resolution_clock::now();
    {
        MatchingEngine engine;
        ThreadPool pool(4, engine);
        for (int i = 0; i < NUM_ORDERS; i++) {
            Order::Side side = (i % 2 == 0) ? Order::Side::Buy : Order::Side::Sell;
            int price = 100 + (i % 10);
            Order o{ i, i, price, 10, side, Order::Type::Limit, Order::TimeInForce::GTC};
            pool.submitOrder(o);
        }
    } // pool destructs before engine here.
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << NUM_ORDERS << " orders in " << duration.count() << " us\n";
    std::cout << "avg: " << static_cast<double>(duration.count()) / NUM_ORDERS << " microseconds/order\n";
    return 0;
}