#include "OrderBook.hpp"

void OrderBook::insertOrder(const Order& order) {
    Order* o = orderPool_.allocateSlot();
    new(o) Order{order};

    if (order.side == Order::Side::Buy) {
        bids_[order.price].push_back(o);
    } else {
        asks_[order.price].push_back(o);
    }

    orderPositionById_[order.id] = o;
}

void OrderBook::cancelOrder(const int orderId) {
    const auto it = orderPositionById_.find(orderId);
    if (it == orderPositionById_.end())
        return;

    Order* o = it->second;

    // erase order and price level if empty in relevant Book Side
    if (o->side == Order::Side::Buy) {
        auto& pLevel = bids_[o->price];
        pLevel.erase(o);
        if (pLevel.empty()) {
            bids_.erase(o->price);
        }
    } else {
        auto& pLevel = asks_[o->price];
        pLevel.erase(o);
        if (pLevel.empty()) {
            asks_.erase(o->price);
        }
    }
    orderPositionById_.erase(it);
    o->~Order();
    orderPool_.deallocateSlot(o);
}

void OrderBook::removeBestBid() {
    auto& topLevel = bids_.begin()->second;
    Order* o = topLevel.head;

    orderPositionById_.erase(o->id);
    topLevel.pop_front();
    if (topLevel.empty())
        bids_.erase(bids_.begin());

    o->~Order();
    orderPool_.deallocateSlot(o);
}

void OrderBook::removeBestAsk() {
    auto& topLevel = asks_.begin()->second;
    Order* o = topLevel.head;

    orderPositionById_.erase(o->id);
    topLevel.pop_front();
    if (topLevel.empty())
        asks_.erase(asks_.begin());

    o->~Order();
    orderPool_.deallocateSlot(o);
}

bool OrderBook::hasBids() const { return !bids_.empty(); }

bool OrderBook::hasAsks() const { return !asks_.empty(); }

Order& OrderBook::getBestBid() const {
    return *bids_.begin()->second.head;
}

Order& OrderBook::getBestAsk() const {
    return *asks_.begin()->second.head;
}

// Returns total quantity available on the ask side that the incoming order can match against.
// Asks are resting SELL orders — sorted lowest price first.
// A buyer (incoming) matches against the cheapest sellers first.
int OrderBook::availableAskQty(const Order& incoming) const {
    int totalQty = 0;

    // asks_ is sorted lowest price first (std::less)
    // a buyer wants the cheapest ask — walk from bottom up
    for (const auto& [price, level] : asks_) {

        // limit order: buyer refuses to pay more than incoming.price
        // once ask price exceeds buyer's limit, no point counting further
        if (incoming.type == Order::Type::Limit && price > incoming.price)
            break;

        // market order: no price limit, count everything available
        for (const Order* resting = level.head; resting != nullptr; resting = resting->next) {
            totalQty += resting->quantity;
        }
    }

    return totalQty;
}

// Returns total quantity available on the bid side that the incoming order can match against.
// Bids are resting BUY orders — sorted highest price first.
// A seller (incoming) matches against the most generous buyers first.
int OrderBook::availableBidQty(const Order& incoming) const {
    int totalQty = 0;

    // bids_ is sorted highest price first (std::greater)
    // a seller wants the highest bid — walk from top down
    for (const auto& [price, level] : bids_) {

        // limit order: seller refuses to accept less than incoming.price
        // once bid price drops below seller's limit, no point counting further
        if (incoming.type == Order::Type::Limit && price < incoming.price)
            break;

        // market order: no price limit, count everything available
        for (const Order* resting = level.head; resting != nullptr; resting = resting->next) {
            totalQty += resting->quantity;
        }
    }

    return totalQty;
}