#include "MatchingEngine.h"
#include <cassert>
#include <iostream>

void test_nonCrossingOrdersRestOnBook() {
    MatchingEngine engine;

    Order buy { 1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC };
    Order sell { 2, 2, 101, 10, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::GTC };

    auto trades1 = engine.matchIncomingOrder(buy);
    auto trades2 = engine.matchIncomingOrder(sell);

    assert(trades1.empty());
    assert(trades2.empty());
}

void test_fifo() {
    MatchingEngine engine;
    Order buy { 1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC };
    Order buy1 { 2, 2, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC };
    Order sell { 3, 3, 100, 10, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::GTC };
    auto trades1 = engine.matchIncomingOrder(buy);
    assert(trades1.empty());
    auto trades2 = engine.matchIncomingOrder(buy1);
    assert(trades2.empty());
    auto trades3 = engine.matchIncomingOrder(sell);
    assert(trades3.size() == 1 && trades3.front().buyOrderId == 1);
}

void test_partialFill() {
    MatchingEngine engine;
    Order buy { 1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC };
    Order sell { 3, 3, 100, 4, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::GTC };
    auto trades = engine.matchIncomingOrder(buy);
    assert(trades.empty());
    auto trades1 = engine.matchIncomingOrder(sell);
    assert(trades1.size() == 1 && trades1.front().quantity == std::min(buy.quantity, sell.quantity));
}

void test_killedFOK() {
    MatchingEngine engine;
    Order buy { 1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC };
    Order sell { 3, 3, 100, 20, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::FOK };
    auto trades1 = engine.matchIncomingOrder(buy);
    // buy should be resting in book and produce 0 trades
    assert(trades1.empty());
    auto trades2 = engine.matchIncomingOrder(sell);
    // sell cannot be filled due to bigger qty.
    assert(trades2.empty());
}

void test_filledFOK() {
    MatchingEngine engine;
    Order buy { 1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC };
    Order sell { 3, 3, 100, 5, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::FOK };
    auto trades1 = engine.matchIncomingOrder(buy);
    // buy should be resting in book and produce 0 trades
    assert(trades1.empty());
    auto trades2 = engine.matchIncomingOrder(sell);
    // sell should be filled due to enough buy qty.
    assert(trades2.size() == 1 && trades2.front().quantity == sell.quantity);
}

// fill whats possible and make sure nothing is left resting on book from IOC order
void test_IOCRemainderDiscarded() {
    MatchingEngine engine;
    Order buy { 1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC };
    Order sell { 3, 3, 100, 20, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::IOC };
    auto trades1 = engine.matchIncomingOrder(buy);
    assert(trades1.empty());
    auto trades2 = engine.matchIncomingOrder(sell);
    // filled ioc amount
    assert(trades2.size() == 1 && trades2.front().quantity == buy.quantity);
    // everything left from sell discarded (not resting on book) so making buy produces 0 trades
    Order buy1 { 4, 4, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC };
    auto trades3 = engine.matchIncomingOrder(buy1);
    assert(trades3.empty());
}

int main() {
    test_nonCrossingOrdersRestOnBook();
    test_fifo();
    test_partialFill();
    test_killedFOK();
    test_filledFOK();
    test_IOCRemainderDiscarded();
    std::cout << "All tests passed" << std::endl;
    return 0;
}