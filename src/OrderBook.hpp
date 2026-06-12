#pragma once

#include "Types.hpp"

#include <list>
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
    void insertOrder(const Order& order);
    void cancelOrder(int orderId);
    // Remove the highest-priority bid and erase the price level
    // if no orders remain at that price.
    void removeBestBid();
    // Remove the highest-priority ask and erase the price level
    // if no orders remain at that price.
    void removeBestAsk();
    [[nodiscard]] bool hasBids() const;
    [[nodiscard]] bool hasAsks() const;
    [[nodiscard]] Order& getBestBid();
    [[nodiscard]] Order& getBestAsk();
    [[nodiscard]] int availableAskQty(const Order& order) const;
    [[nodiscard]] int availableBidQty(const Order& order) const;

private:
    std::map<int, std::list<Order>, std::greater<>>         bidLevels_;
    std::map<int, std::list<Order>>                         askLevels_;
    std::unordered_map<int, std::list<Order>::iterator>     orderPositionById_;
};
