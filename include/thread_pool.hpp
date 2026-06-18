#pragma once
#include "matching_engine.hpp"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// Manages a pool of worker threads that process incoming orders.
// Borrows a MatchingEngine reference — the engine must outlive the pool.
class thread_pool {
public:
    explicit thread_pool(int numThreads, matching_engine& engine);
    ~thread_pool();
    thread_pool(const thread_pool&)               = delete;
    thread_pool& operator=(const thread_pool&)    = delete;
    thread_pool(thread_pool&&)                    = delete;
    thread_pool& operator=(thread_pool&&)         = delete;
    // Submit order for asynchronous processing by worker threads.
    void submitOrder(Order order);

private:
    void workerThreadLoop();
    // ThreadPool borrows the MatchingEngine. Someone else created it (main)
    // and will destroy it. ThreadPool just has a way to reach it.
    matching_engine&             engine_;
    std::queue<Order>           orderQueue_;
    std::mutex                  queueMutex_;
    std::condition_variable     queueCv_;
    std::vector<std::thread>    workers_;
    std::atomic<bool>           shutdown_ {false};
};
