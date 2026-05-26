#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

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
    int buyOrderID;
    int sellOrderID;

    int price;
    int quantity;
};

// ================================================================================================
// ================================================================================================
// ================================================================================================

// Stores active unmatched orders
class OrderBook {
public:
    void printBook() const {
        std::cout << "BIDS:\n";
        for (const auto& [price, orders] : bids_) {
            std::cout << "  " << price << ": ";
            for (const auto& o : orders) {
                std::cout << "[id=" << o.id << " qty=" << o.quantity << "] ";
            }
            std::cout << '\n';
        }
        std::cout << "ASKS:\n";
        for (const auto& [price, orders] : asks_) {
            std::cout << "  " << price << ": ";
            for (const auto& o : orders) {
                std::cout << "[id=" << o.id << " qty=" << o.quantity << "] ";
            }
            std::cout << '\n';
        }
    }

    void add(const Order& order) {
        if (order.side == Order::Side::Buy) {
            // store order in orderbook
            auto& orderList = bids_[order.price];
            orderList.push_back(order);
            // store order in map for fast lookup in cancel()
            orderIdToIterator_[order.id] = std::prev(orderList.end());
        } else {
            auto& orderList = asks_[order.price];
            orderList.push_back(order);
            // store order in map for fast lookup in cancel()
            orderIdToIterator_[order.id] = std::prev(orderList.end());
        }
    }

    void cancel(const int orderId) {
        // get iterator to entry in map of std::list
        const auto it = orderIdToIterator_.find(orderId);
        // make sure its valid (order exists)
        if (it == orderIdToIterator_.end()) { return; }

        // get order from std::list iterator
        const Order order = *it->second;

        // find which side of orderbook order is in and remove it
        if (order.side == Order::Side::Buy) {
            auto& orderList = bids_[order.price];
            orderList.erase(it->second);
            // make sure to check incase price level empty after cancelling order
            if (orderList.empty()) { bids_.erase(order.price); }
        } else {
            auto& orderList = asks_[order.price];
            orderList.erase(it->second);
            if (orderList.empty()) { asks_.erase(order.price); }
        }
        // finally remove from iterator map
        orderIdToIterator_.erase(orderId);
    }

    void popBuy() {
        const auto level = bids_.begin();
        Order& order = level->second.front();
        // remove order from orderIdToIterator map
        orderIdToIterator_.erase(order.id);
        // remove from orderbook
        level->second.pop_front();
        // make sure we delete price level from orderbook if empty after removing order
        if (level->second.empty()) {
            bids_.erase(level);
        }
    }

    void popSell() {
        const auto level = asks_.begin();
        Order& order = level->second.front();
        // remove order from orderIdToIterator map
        orderIdToIterator_.erase(order.id);
        // remove from orderbook
        level->second.pop_front();
        // make sure we delete price level from orderbook if empty after removing order
        if (level->second.empty()) {
            asks_.erase(level);
        }
    }

    [[nodiscard]] bool hasBuys() const { return !bids_.empty(); }

    [[nodiscard]] bool hasSells() const { return !asks_.empty(); }

    [[nodiscard]] Order topBuy() const {
        const auto level = bids_.begin();       // highest price
        return level->second.front();           // oldest order (first/left in deque)
    }
    [[nodiscard]] Order topSell() const {
        const auto level = asks_.begin();       // lowest price
        return level->second.front();           // oldest order (first/left in deque)
    }

private:
    // bids_: highest price first → use std::greater
    std::map<int, std::list<Order>, std::greater<>>         bids_;
    // asks_: lowest price first → default std::less is fine
    std::map<int, std::list<Order>>                         asks_;
    // maps orderId : iterator pointing to Order for easy price level lookup in cancel()
    // doubly linked list so removing Order from middle is easy (std::deque would shift and leave dangling iterators)
    std::unordered_map<int, std::list<Order>::iterator>     orderIdToIterator_;
};


// ================================================================================================
// ================================================================================================
// ================================================================================================

// Performs matching on orders inside orderbook
class MatchingEngine {
public:
    // passthrough functions since engine owns orderbook
    void printBook() const { book_.printBook(); }
    void cancel(const int orderId) { book_.cancel(orderId); }

    std::vector<Trade> process(Order incomingOrder) {
        std::vector<Trade> trades;
        if (incomingOrder.side == Order::Side::Buy) {
            processBuy(incomingOrder, trades);
        } else {
            processSell(incomingOrder, trades);
        }
        return trades;
    }

private:
    // MatchingEngine creates, owns and destroys the OrderBook.
    OrderBook book_;

    void processBuy(Order& buyOrder, std::vector<Trade>& trades) {
        while (book_.hasSells()) {
            Order bestSell = book_.topSell();

            if (!canMatch(buyOrder, bestSell)) { break; }

            int const quantityTraded = std::min(buyOrder.quantity, bestSell.quantity);
            trades.push_back(
                Trade{
                    buyOrder.id,
                    bestSell.id,
                    bestSell.price,
                    quantityTraded
                });
            buyOrder.quantity -= quantityTraded;
            bestSell.quantity -= quantityTraded;

            book_.popSell();
            if (bestSell.quantity > 0 ) { book_.add(bestSell); }
            if (buyOrder.quantity == 0) { return; }
        }
        // Whatever is left of this order could not be matched, so it stays in the book.
        book_.add(buyOrder);
    }

    void processSell(Order& sellOrder, std::vector<Trade>& trades) {
        while (book_.hasBuys()) {
            Order bestBuy = book_.topBuy();

            if (!canMatch(bestBuy, sellOrder)) { break; }

            const int quantityTraded = std::min(bestBuy.quantity, sellOrder.quantity);

            trades.push_back(
                Trade{
                    bestBuy.id,
                    sellOrder.id,
                    bestBuy.price,
                    quantityTraded
                }
            );

            bestBuy.quantity -= quantityTraded;
            sellOrder.quantity -= quantityTraded;

            book_.popBuy();
            if (bestBuy.quantity > 0) { book_.add(bestBuy); }
            if (sellOrder.quantity == 0) { return; }
        }
        book_.add(sellOrder);
    }

    static bool canMatch(const Order& buyOrder, const Order& sellOrder) {
        // match market orders
        if (buyOrder.type == Order::Type::Market || sellOrder.type == Order::Type::Market) {
            return true;
        }
        // match limit orders
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
        // fill threads_ with numthreads worker threads
        for (int i = 0; i < numThreads; ++i) {
            threads_.emplace_back([this]() {    // 'this' gives access to all mem. variables
                // cv.wait() requires a unique_lock and allows unlocking (also nice RAII)
                std::unique_lock<std::mutex> lock(mutex_);
                while (!stopped_ || !queue_.empty()) {
                    // release lock and sleep until predicate returns true
                    cv_.wait(lock, [this]() {
                        return !queue_.empty() || stopped_;
                    });
                    // acquire lock and do work
                    if (!queue_.empty()) {
                        // copy assignment because pop() would cause dangling reference
                        Order order = queue_.front();
                        queue_.pop();
                        lock.unlock();
                        engine_.process(order); // dependency injection
                        lock.lock();
                    }
                }
            });
        }
    }
    ~ThreadPool();

    // delete copy
    ThreadPool(const ThreadPool&)               = delete;
    ThreadPool& operator=(const ThreadPool&)    = delete;

    // delete move
    ThreadPool(ThreadPool&&)                    = delete;
    ThreadPool& operator=(ThreadPool&&)         = delete;

    // push an order onto the queue
    void enqueue(Order order) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(order);
        cv_.notify_one();
    }

    // tell all threads to shut down
    void stop();

private:
    // ThreadPool borrows the MatchingEngine. Someone else created it (main)
    // and will destroy it. ThreadPool just has a way to reach it.
    MatchingEngine&             engine_;
    std::queue<Order>           queue_;
    std::mutex                  mutex_;
    std::condition_variable     cv_;
    std::vector<std::thread>    threads_;
    bool                        stopped_ {false};
};

// ================================================================================================
// ================================================================================================
// ================================================================================================


int main() {
    MatchingEngine engine;
    // benchmark
    constexpr int NUM_ORDERS = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_ORDERS; i++) {
        Order o{ i, i, 100 + (i % 10), 10, Order::Side::Buy, Order::Type::Limit };
        engine.process(o);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << NUM_ORDERS << " orders in " << duration.count() << " us\n";
    std::cout << "avg: " << static_cast<double>(duration.count()) / NUM_ORDERS << " us/order\n";
    return 0;
}