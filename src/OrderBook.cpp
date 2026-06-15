#include "OrderBook.hpp"

void OrderBook::insertOrder(const Order& order) {
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

void OrderBook::cancelOrder(const int orderId) {
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
void OrderBook::removeBestBid() {
    const auto lvl = bidLevels_.begin();
    const Order& order = lvl->second.front();
    orderPositionById_.erase(order.id);
    lvl->second.pop_front();

    if (lvl->second.empty()) { bidLevels_.erase(lvl); }
}

// Remove top-of-book order and cleanup empty price level
void OrderBook::removeBestAsk() {
    const auto lvl = askLevels_.begin();
    const Order& order = lvl->second.front();
    orderPositionById_.erase(order.id);
    lvl->second.pop_front();

    if (lvl->second.empty()) { askLevels_.erase(lvl); }
}

bool OrderBook::hasBids() const { return !bidLevels_.empty(); }

bool OrderBook::hasAsks() const { return !askLevels_.empty(); }

Order& OrderBook::getBestBid() {
    const auto level = bidLevels_.begin();
    return level->second.front();
}

Order& OrderBook::getBestAsk() {
    const auto level = askLevels_.begin();
    return level->second.front();
}

int OrderBook::availableAskQty(const Order& order) const {
    int total = 0;
    for (const auto& [price, orders] : askLevels_) {
        if (order.type == Order::Type::Limit && price > order.price) break;
        for (const auto& o : orders) {
            total += o.quantity;
        }
    }
    return total;
}

int OrderBook::availableBidQty(const Order& order) const {
    int total = 0;
    for (const auto& [price, orders] : bidLevels_) {
        if (order.type == Order::Type::Limit && price < order.price) break;
        for (const auto& o : orders) {
            total += o.quantity;
        }
    }
    return total;
}