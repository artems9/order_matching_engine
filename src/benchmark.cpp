#include <iostream>
#include <vector>
#include "MatchingEngine.h"

int main() {
    std::vector<long> latencies;
    constexpr int NUM_ORDERS = 1000000;
    latencies.reserve(NUM_ORDERS);

    MatchingEngine engine;
    for (int i = 0; i < NUM_ORDERS; i++) {
        auto t1 = std::chrono::high_resolution_clock::now();
        Order::Side side = (i % 2 == 0) ? Order::Side::Buy : Order::Side::Sell;
        int price = 100 + (i % 10);
        Order o{ i, i, price, 10, side, Order::Type::Limit, Order::TimeInForce::GTC};
        engine.matchIncomingOrder(o);
        auto t2 = std::chrono::high_resolution_clock::now();
        latencies.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()
        );
    }

    std::sort(latencies.begin(), latencies.end());
    std::cout << "p50: " << latencies[NUM_ORDERS * 0.50] << " ns\n";
    std::cout << "p95: " << latencies[NUM_ORDERS * 0.95] << " ns\n";
    std::cout << "p99: " << latencies[NUM_ORDERS * 0.99] << " ns\n";
}