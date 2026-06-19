#include "../include/matching_engine.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

int main() {
    std::vector<long> latencies;
    constexpr int NUM_ORDERS = 1000000;
    latencies.reserve(NUM_ORDERS);

    auto totalStart = std::chrono::high_resolution_clock::now();
    matching_engine engine;
    for (int i = 0; i < NUM_ORDERS; i++) {
        Order::Side side = (i % 2 == 0) ? Order::Side::Buy : Order::Side::Sell;
        int price = 100 + (i % 10);
        Order o{i, i, price, 10, side, Order::Type::Limit, Order::TimeInForce::GTC};
        auto start = std::chrono::high_resolution_clock::now();
        engine.matchIncomingOrder(o);
        auto end = std::chrono::high_resolution_clock::now();
        latencies.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
            );
    }
    auto totalEnd = std::chrono::high_resolution_clock::now();
    auto totalUs = std::chrono::duration_cast<std::chrono::microseconds>(totalEnd - totalStart).
        count();
    std::cout << "throughput: " << (NUM_ORDERS / (totalUs / 1e6)) << " orders/sec\n";

    std::sort(latencies.begin(), latencies.end());
    std::cout << "p50: " << latencies[static_cast<size_t>(NUM_ORDERS * 0.50)] << " ns\n";
    std::cout << "p95: " << latencies[static_cast<size_t>(NUM_ORDERS * 0.95)] << " ns\n";
    std::cout << "p99: " << latencies[static_cast<size_t>(NUM_ORDERS * 0.99)] << " ns\n";
}