#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>

// Represents a request to buy/sell
struct Order {
    enum class Side {
        Buy,
        Sell
    };

    enum class Type {
        Market,
        Limit
    };

    int id;
    int seqNum;
    int price;
    int quantity;

    Side side;
    Type type;
};

// Represents an executed transaction
struct Trade {
    int buyOrderId;
    int sellOrderId;

    int price;
    int quantity;
};


// OrderBook implements price-time priority matching:
// - Bids: highest price first, FIFO within each price level
// - Asks: lowest price first, FIFO within each price level
// - Orders at the same price execute in arrival order (oldest first)
class OrderBook {
public:
    void insertOrder(const Order& order) {
        if (order.side == Order::Side::Buy) {
            auto& orderList = bidLevels_[order.price];
            orderList.push_back(order);
            orderPositionById_[order.id] = std::prev(orderList.end());
        } else {
            auto& orderList = askLevels_[order.price];
            orderList.push_back(order);
            orderPositionById_[order.id] = std::prev(orderList.end());
        }
    }

    void cancelOrder(const int orderId) {
        const auto it = orderPositionById_.find(orderId);
        if (it == orderPositionById_.end()) { return; }
        const Order order = *it->second;

        if (order.side == Order::Side::Buy) {
            auto& lvl = bidLevels_[order.price];
            lvl.erase(it->second);
            if (lvl.empty()) { bidLevels_.erase(order.price); }
        } else {
            auto& lvl = askLevels_[order.price];
            lvl.erase(it->second);
            if (lvl.empty()) { askLevels_.erase(order.price); }
        }
        orderPositionById_.erase(orderId);
    }

    // Remove top-of-book order and cleanup empty price level
    void removeBestBid() {
        const auto lvl = bidLevels_.begin();
        const Order& order = lvl->second.front();
        orderPositionById_.erase(order.id);
        lvl->second.pop_front();

        if (lvl->second.empty()) { bidLevels_.erase(lvl); }
    }

    // Remove top-of-book order and cleanup empty price level
    void removeBestAsk() {
        const auto lvl = askLevels_.begin();
        const Order& order = lvl->second.front();
        orderPositionById_.erase(order.id);
        lvl->second.pop_front();

        if (lvl->second.empty()) { askLevels_.erase(lvl); }
    }

    [[nodiscard]] bool hasBids() const { return !bidLevels_.empty(); }

    [[nodiscard]] bool hasAsks() const { return !askLevels_.empty(); }

    [[nodiscard]] Order& getBestBid() {
        const auto level = bidLevels_.begin();
        return level->second.front();
    }

    [[nodiscard]] Order& getBestAsk() {
        const auto level = askLevels_.begin();
        return level->second.front();
    }

private:
    std::map<int, std::list<Order>, std::greater<>>         bidLevels_;
    std::map<int, std::list<Order>>                         askLevels_;
    // Index order position for O(1) cancellation by orderId.
    std::unordered_map<int, std::list<Order>::iterator>     orderPositionById_;
};


// ================================================================================================

// Matches incoming order against resting liquidity and returns executed trades
class MatchingEngine {
public:
    std::vector<Trade> matchIncomingOrder(Order incomingOrder) {
        std::lock_guard<std::mutex> lock(engineMutex_);
        std::vector<Trade> trades;
        if (incomingOrder.side == Order::Side::Buy) {
            matchIncomingBuy(incomingOrder, trades);
        } else {
            matchIncomingSell(incomingOrder, trades);
        }
        return trades;
    }

private:
    OrderBook book_;    // MatchingEngine creates, owns and destroys the OrderBook.
    std::mutex engineMutex_;

    void matchIncomingBuy(Order& buyOrder, std::vector<Trade>& trades) {
        while (book_.hasAsks()) {
            // Resting order is partially filled; only removed if fully consumed.
            Order& restingAsk = book_.getBestAsk();

            if (!canMatch(buyOrder, restingAsk)) { break; }

            int const quantityTraded = std::min(buyOrder.quantity, restingAsk.quantity);
            trades.push_back(
                Trade{
                    buyOrder.id,
                    restingAsk.id,
                    restingAsk.price,
                    quantityTraded
                });
            buyOrder.quantity -= quantityTraded;
            restingAsk.quantity -= quantityTraded;

            if (restingAsk.quantity == 0 ) { book_.removeBestAsk(); }
            if (buyOrder.quantity == 0) { return; }
        }
        // Unfilled limit orders rest in the book; market orders are discarded if unfilled.
        if (buyOrder.type == Order::Type::Limit) {
            book_.insertOrder(buyOrder);
        }
    }

    void matchIncomingSell(Order& sellOrder, std::vector<Trade>& trades) {
        while (book_.hasBids()) {
            Order& restingBid = book_.getBestBid();

            if (!canMatch(restingBid, sellOrder)) { break; }

            const int tradeQty = std::min(restingBid.quantity, sellOrder.quantity);

            trades.push_back(
                Trade{
                    restingBid.id,
                    sellOrder.id,
                    restingBid.price,
                    tradeQty
                }
            );

            restingBid.quantity -= tradeQty;
            sellOrder.quantity -= tradeQty;

            if (restingBid.quantity == 0) { book_.removeBestBid(); }
            if (sellOrder.quantity == 0) { return; }
        }
        if (sellOrder.type == Order::Type::Limit) {
            book_.insertOrder(sellOrder);
        }
    }

    static bool canMatch(const Order& buyOrder, const Order& sellOrder) {
        // Market orders cross the spread regardless of price; only liquidity limits matching.
        if (buyOrder.type == Order::Type::Market && sellOrder.type == Order::Type::Limit) {
            return true;
        }
        if (sellOrder.type == Order::Type::Market && buyOrder.type == Order::Type::Limit) {
            return true;
        }
        // Matching condition: bid >= ask (price-time priority applies after this check).
        return buyOrder.price >= sellOrder.price;
    }

};

// ================================================================================================
// ================================================================================================
// ================================================================================================

// Manages worker threads that process orders in a queue
class ThreadPool {
public:
    explicit ThreadPool(const int numThreads, MatchingEngine& engine) : engine_(engine) {
        workers_.reserve(numThreads);
        for (int i = 0; i < numThreads; ++i) {
            workers_.emplace_back(&ThreadPool::workerThreadLoop, this);
        }
    }


    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            shutdown_ = true;
        }
        queueCv_.notify_all();
        // Join all workers to ensure no thread accesses destroyed engine or queue.
        for (auto& t : workers_) {
            if (t.joinable()) { t.join(); }
        }
    }

    // delete copy
    ThreadPool(const ThreadPool&)               = delete;
    ThreadPool& operator=(const ThreadPool&)    = delete;

    // delete move
    ThreadPool(ThreadPool&&)                    = delete;
    ThreadPool& operator=(ThreadPool&&)         = delete;

    // Submit order for asynchronous processing by worker threads.
    void submitOrder(Order order) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        orderQueue_.push(order);
        queueCv_.notify_one();
    }

private:
    void workerThreadLoop() {
        while (true) {
            Order order;
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                queueCv_.wait(lock, [this]() {
                    return !orderQueue_.empty() || shutdown_;
                });
                if (shutdown_ && orderQueue_.empty()) return;
                order = orderQueue_.front();
                orderQueue_.pop();
            }
            engine_.matchIncomingOrder(order);
        }
    }

    // ThreadPool borrows the MatchingEngine. Someone else created it (main)
    // and will destroy it. ThreadPool just has a way to reach it.
    MatchingEngine&             engine_;
    std::queue<Order>           orderQueue_;
    std::mutex                  queueMutex_;
    std::condition_variable     queueCv_;
    std::vector<std::thread>    workers_;
    bool                        shutdown_ {false};
};

// ================================================================================================
// ================================================================================================
// ================================================================================================


int main() {
    // benchmark
    constexpr int NUM_ORDERS = 1000000;
    auto start = std::chrono::high_resolution_clock::now();
    {
        MatchingEngine engine;
        ThreadPool pool(4, engine);
        for (int i = 0; i < NUM_ORDERS; i++) {
            Order::Side side = (i % 2 == 0) ? Order::Side::Buy : Order::Side::Sell;
            int price = 100 + (i % 10);
            Order o{ i, i, price, 10, side, Order::Type::Limit };
            pool.submitOrder(o);
        }
    } // pool destructs before engine here.
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << NUM_ORDERS << " orders in " << duration.count() << " us\n";
    std::cout << "avg: " << static_cast<double>(duration.count()) / NUM_ORDERS << " microseconds/order\n";
    return 0;
}