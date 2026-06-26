#include "../include/ThreadPool.hpp"

ThreadPool::ThreadPool(const int numThreads, MatchingEngine& engine) : engine_(engine) {
    workers_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ThreadPool::workerThreadLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown_ = true;
    queueCv_.notify_all();
    // Join all workers to ensure no thread accesses destroyed engine or queue.
    for (auto& t : workers_) {
        if (t.joinable()) { t.join(); }
    }
}

void ThreadPool::submitOrder(Order order) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    orderQueue_.push(order);
    queueCv_.notify_one();
}

void ThreadPool::workerThreadLoop() {
    while (true) {
        Order order;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCv_.wait(lock, [this]() {
                return !orderQueue_.empty() || shutdown_;
            });
            if (shutdown_ && orderQueue_.empty())
                return;
            order = orderQueue_.front();
            orderQueue_.pop();
        }
        engine_.matchIncomingOrder(order);
    }
}