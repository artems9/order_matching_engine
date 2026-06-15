#include "MatchingEngine.hpp"

// Trades execute at the resting order's price.
// The resting order has time priority and therefore
// determines the execution price.
std::vector<Trade> MatchingEngine::matchIncomingOrder(Order incomingOrder) {
    std::lock_guard<std::mutex> lock(engineMutex_);
    std::vector<Trade> trades;
    trades.reserve(8);
    if (incomingOrder.side == Order::Side::Buy) {
        matchIncomingBuy(incomingOrder, trades);
    } else {
        matchIncomingSell(incomingOrder, trades);
    }
    return trades;
}

void MatchingEngine::executeTrade(Order& buyOrder, Order& sellOrder, const int executionPrice, std::vector<Trade>& trades) {
    const int tradeQty = std::min(buyOrder.quantity, sellOrder.quantity);
    trades.push_back({ buyOrder.id, sellOrder.id, executionPrice, tradeQty });
    buyOrder.quantity -= tradeQty;
    sellOrder.quantity -= tradeQty;
}

// GTC + Limit   → insertOrder
// GTC + Market  → discard
// IOC           → discard
// FOK           → nothing (already fully filled or killed before loop)
bool MatchingEngine::canFullyFill(const Order& order) const {
    int available = (order.side == Order::Side::Buy)
        ? book_.availableAskQty(order)
        : book_.availableBidQty(order);

    return available >= order.quantity;
}

void MatchingEngine::matchIncomingBuy(Order& buyOrder, std::vector<Trade>& trades) {
    // FOK: check before touching the book at all
    if (buyOrder.tif == Order::TimeInForce::FOK && !canFullyFill(buyOrder)) { return; }

    // match against resting asks
    while (book_.hasAsks()) {
        Order& restingAsk = book_.getBestAsk();
        if (!canMatch(buyOrder, restingAsk)) { break; }
        executeTrade(buyOrder, restingAsk, restingAsk.price, trades);
        if (restingAsk.quantity == 0) { book_.removeBestAsk(); }
        if (buyOrder.quantity == 0) { return; }
    }

    // handle whatever is left
    handleRemainder(buyOrder);
}

void MatchingEngine::handleRemainder(const Order& order) {
    switch (order.tif) {
    case Order::TimeInForce::GTC:
        if (order.type == Order::Type::Limit) {
            book_.insertOrder(order);
        }
        break;
    case Order::TimeInForce::IOC:
        break; // discard remainder
    case Order::TimeInForce::FOK:
        break; // fully filled or already returned early
    }
}

void MatchingEngine::matchIncomingSell(Order& sellOrder, std::vector<Trade>& trades) {
    // Handle FOK orders
    if (sellOrder.tif == Order::TimeInForce::FOK && !canFullyFill(sellOrder)) { return; }

    while (book_.hasBids()) {
        Order& restingBid = book_.getBestBid();
        if (!canMatch(restingBid, sellOrder)) { break; }
        executeTrade(restingBid, sellOrder, restingBid.price, trades);
        // Remove empty price level once the resting order
        // has been completely consumed.
        if (restingBid.quantity == 0) { book_.removeBestBid(); }
        // Incoming order fully filled.
        if (sellOrder.quantity == 0) { return; }
    }

    handleRemainder(sellOrder);
}

bool MatchingEngine::canMatch(const Order& buyOrder, const Order& sellOrder) {
    // Market orders cross the spread regardless of price; only liquidity limits matching.
    if (buyOrder.type == Order::Type::Market && sellOrder.type == Order::Type::Limit) {
        return true;
    }
    if (sellOrder.type == Order::Type::Market && buyOrder.type == Order::Type::Limit) {
        return true;
    }
    // Matching condition: bid >= ask (price-time priority applies after this check).
    // Market vs Market: not supported in practice; both have price 0 so they match.
    return buyOrder.price >= sellOrder.price;
}
