#pragma once
#include "OrderBook.hpp"
#include "Types.hpp"
#include <mutex>
#include <vector>

// Matches incoming orders against resting liquidity using
// price-time priority.
//
// Matching rules:
// - Buy orders match the best available asks.
// - Sell orders match the best available bids.
// - Orders at the same price execute FIFO.
// - Unfilled limit orders rest on the book.
// - Unfilled market orders are discarded.

class MatchingEngine {
public:
    // Processes an incoming order and returns all trades
    // generated during matching.
    std::vector<Trade> matchIncomingOrder(Order incomingOrder);

private:
    // MatchingEngine creates, owns and destroys the OrderBook.
    OrderBook book_;
    std::mutex engineMutex_;

    bool canFullyFill(const Order& order) const;
    void handleRemainder(const Order& order, std::vector<Trade>& trades);
    void matchIncomingBuy(Order& buyOrder, std::vector<Trade>& trades);
    void matchIncomingSell(Order& sellOrder, std::vector<Trade>& trades);
    static bool canMatch(const Order& buyOrder, const Order& sellOrder);
    static void executeTrade(Order& buyOrder, Order& sellOrder, int executionPrice, std::vector<Trade>& trades);
};