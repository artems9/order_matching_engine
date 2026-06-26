#pragma once

#include "Types.hpp"
#include "OrderPool.hpp"

#include <map>
#include <unordered_map>

// Maintains a price-time-priority limit order book.
//
// Bids:
//   highest price first
//
// Asks:
//   lowest price first
//
// Orders at the same price execute FIFO.
class OrderBook {
public:
    OrderBook() = default;
    ~OrderBook() = default;

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    // will never move in practice therefore marked delete
    OrderBook(OrderBook&&) = delete;
    OrderBook& operator=(OrderBook&&) = delete;

    void insertOrder(const Order& order);
    // Cancel a resting order by ID in O(1) time. No-op if the order is not found.
    void cancelOrder(int orderId);
    // Remove the highest-priority bid and erase the price level
    // if no orders remain at that price.
    void removeBestBid();
    // Remove the highest-priority ask and erase the price level
    // if no orders remain at that price.
    void removeBestAsk();

    [[nodiscard]] bool hasBids() const;
    [[nodiscard]] bool hasAsks() const;
    [[nodiscard]] Order& getBestBid() const;
    [[nodiscard]] Order& getBestAsk() const;
    [[nodiscard]] int availableAskQty(const Order& order) const;
    [[nodiscard]] int availableBidQty(const Order& order) const;

private:
    // doubly linked list because cancel() takes pointer to Order that may be in middle
    // allows O(1) for all operations on any Order in PriceLevel
    // most importantly, avoid heap malloc for every order insert
    // MANAGES HOW ORDERS ARE CONNECTED TO EACH OTHER
    struct PriceLevel {
        Order* head{nullptr};
        Order* tail{nullptr};

        // insert at back - maintains time priority (FIFO)
        void push_back(Order* o) {
            o->prev = tail;
            o->next = nullptr;

            if (empty()) {
                head = o; // first order
            } else {
                tail->next = o; // link old tail to new order
            }
            tail = o;
        }

        // remove from front — used when matching best order
        // front order has waited longest at this price (time priority)
        void pop_front() {
            assert(head && "pop_front() called on empty PriceLevel");
            head = head->next;
            if (head) {
                head->prev = nullptr; // new head
            } else {
                tail = nullptr; // list is empty
            }
        }

        // remove any order by pointer — used for cancel
        // O(1) because Order carries prev/next pointers (doubly linked)
        void erase(Order* o) {
            if (o == head) {
                // case first in level
                pop_front();
            } else if (o == tail) {
                // case last in level
                // fix prev Orders forward link
                Order* const prev = o->prev;
                prev->next = nullptr;
                // move tail back
                tail = prev;
            } else {
                // case middle in level
                // fix prev Orders forward link
                Order* const prev = o->prev;
                prev->next = o->next;
                // fix next Orders backward link
                Order* const next = o->next;
                next->prev = o->prev;
            }
        }

        // returns front order — oldest order at this price level
        [[nodiscard]] Order* front() const {
            assert(head && "front() called on empty PriceLevel");
            return head;
        }

        [[nodiscard]] bool empty() const { return head == nullptr; }

    };

    //  Powers of 2 play nicely with memory alignment,
    //  cache line sizes, and CPU prefetchers.
    //  Also, easier to reason about — 65536 = 2^16
    static constexpr std::size_t MAX_NUM_ORDERS{65536};
    // OrderBook manages OrderPool - composition
    OrderPool<MAX_NUM_ORDERS> orderPool_;

    // std::list replaced with PriceLevel
    // BIDS  = buyers  = want LOW price = sorted HIGH first (best bid = highest)
    // ASKS  = sellers = want HIGH price = sorted LOW first  (best ask = lowest)
    // A trade happens when the highest bid meets the lowest ask. That's the spread.
    std::map<int, PriceLevel, std::greater<>> bids_; // highest first
    std::map<int, PriceLevel, std::less<>> asks_; // lowest first

    // iterator replaced with pointer to Order in PriceLevel using OrderId
    std::unordered_map<int, Order*> orderPositionById_;
};