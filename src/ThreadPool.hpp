#pragma once
#include "MatchingEngine.hpp"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class MatchingEngine;  // forward declaration

// Manages worker threads that process orders in a queue
class ThreadPool {
public:
    explicit ThreadPool(int numThreads, MatchingEngine& engine);
    ~ThreadPool();
    ThreadPool(const ThreadPool&)               = delete;
    ThreadPool& operator=(const ThreadPool&)    = delete;
    ThreadPool(ThreadPool&&)                    = delete;
    ThreadPool& operator=(ThreadPool&&)         = delete;
    // Submit order for asynchronous processing by worker threads.
    void submitOrder(Order order);

private:
    void workerThreadLoop();
    // ThreadPool borrows the MatchingEngine. Someone else created it (main)
    // and will destroy it. ThreadPool just has a way to reach it.
    MatchingEngine&             engine_;
    std::queue<Order>           orderQueue_;
    std::mutex                  queueMutex_;
    std::condition_variable     queueCv_;
    std::vector<std::thread>    workers_;
    bool                        shutdown_ {false};
};
