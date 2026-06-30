#include "MatchingEngine.hpp"
#include "OrderBook.hpp"
#include <gtest/gtest.h>

// ===================================================================
// MatchingEngine — order matching logic
// ===================================================================

TEST(MatchingEngineTest, NonCrossingOrdersRestOnBook) {
    MatchingEngine engine;

    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order sell{2, 2, 101, 10, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::GTC};

    auto trades1 = engine.matchIncomingOrder(buy);
    auto trades2 = engine.matchIncomingOrder(sell);

    EXPECT_TRUE(trades1.empty());
    EXPECT_TRUE(trades2.empty());
}

TEST(MatchingEngineTest, FIFO) {
    MatchingEngine engine;
    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order buy1{2, 2, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order sell{3, 3, 100, 10, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::GTC};

    EXPECT_TRUE(engine.matchIncomingOrder(buy).empty());
    EXPECT_TRUE(engine.matchIncomingOrder(buy1).empty());

    auto trades = engine.matchIncomingOrder(sell);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades.front().buyOrderId, 1) << "oldest resting order should match first";
}

TEST(MatchingEngineTest, PartialFill) {
    MatchingEngine engine;
    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order sell{3, 3, 100, 4, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::GTC};

    EXPECT_TRUE(engine.matchIncomingOrder(buy).empty());

    auto trades = engine.matchIncomingOrder(sell);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades.front().quantity, std::min(buy.quantity, sell.quantity));
}

TEST(MatchingEngineTest, FOK_KilledWhenNotFullyFillable) {
    MatchingEngine engine;
    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order sell{3, 3, 100, 20, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::FOK};

    EXPECT_TRUE(engine.matchIncomingOrder(buy).empty());

    auto trades = engine.matchIncomingOrder(sell);
    EXPECT_TRUE(trades.empty()) << "FOK should be killed entirely when quantity exceeds liquidity";
}

TEST(MatchingEngineTest, FOK_FilledWhenFullyFillable) {
    MatchingEngine engine;
    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order sell{3, 3, 100, 5, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::FOK};

    EXPECT_TRUE(engine.matchIncomingOrder(buy).empty());

    auto trades = engine.matchIncomingOrder(sell);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades.front().quantity, sell.quantity);
}

TEST(MatchingEngineTest, IOC_RemainderDiscarded) {
    MatchingEngine engine;
    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order sell{3, 3, 100, 20, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::IOC};

    EXPECT_TRUE(engine.matchIncomingOrder(buy).empty());

    auto trades = engine.matchIncomingOrder(sell);
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades.front().quantity, buy.quantity);

    Order buy1{4, 4, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    EXPECT_TRUE(engine.matchIncomingOrder(buy1).empty())
        << "IOC remainder should have been discarded, not resting on book";
}

TEST(MatchingEngineTest, MarketOrderMatchesAgainstBestPrice) {
    MatchingEngine engine;
    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    EXPECT_TRUE(engine.matchIncomingOrder(buy).empty());

    Order sell{2, 2, 0, 10, Order::Side::Sell, Order::Type::Market, Order::TimeInForce::GTC};
    auto trades = engine.matchIncomingOrder(sell);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades.front().quantity, 10);
}

TEST(MatchingEngineTest, MatchesBestPriceAcrossMultipleLevels) {
    MatchingEngine engine;

    Order buy1{1, 1, 102, 5, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order buy2{2, 2, 100, 5, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    EXPECT_TRUE(engine.matchIncomingOrder(buy1).empty());
    EXPECT_TRUE(engine.matchIncomingOrder(buy2).empty());

    Order sell{3, 3, 99, 5, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::GTC};
    auto trades = engine.matchIncomingOrder(sell);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades.front().buyOrderId, 1) << "should match the highest bid first";
}

// ===================================================================
// OrderBook — cancellation and book state
// ===================================================================

TEST(OrderBookTest, CancelledOrderDoesNotMatch) {
    OrderBook book;

    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    book.insertOrder(buy);
    book.cancelOrder(1);

    EXPECT_FALSE(book.hasBids()) << "book should be empty after cancelling its only order";
}

TEST(OrderBookTest, CancelNonexistentOrderIsNoOp) {
    OrderBook book;
    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    book.insertOrder(buy);

    book.cancelOrder(999);

    EXPECT_TRUE(book.hasBids()) << "cancelling an unknown id should not affect the book";
}

TEST(OrderBookTest, CancelMiddleOrderPreservesRemainingFIFOOrder) {
    OrderBook book;
    Order a{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order b{2, 2, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order c{3, 3, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};

    book.insertOrder(a);
    book.insertOrder(b);
    book.insertOrder(c);
    book.cancelOrder(2);

    EXPECT_EQ(book.getBestBid().id, 1);
    book.removeBestBid();
    EXPECT_EQ(book.getBestBid().id, 3) << "order 2 should have been unlinked, skipping straight to order 3";
}

TEST(OrderBookTest, PriceLevelRemovedWhenEmptied) {
    OrderBook book;
    Order buy{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    book.insertOrder(buy);
    book.removeBestBid();

    EXPECT_FALSE(book.hasBids()) << "price level should be erased once its last order is removed";
}

TEST(OrderBookTest, BestBidIsHighestPrice) {
    OrderBook book;
    Order low{1, 1, 100, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};
    Order high{2, 2, 105, 10, Order::Side::Buy, Order::Type::Limit, Order::TimeInForce::GTC};

    book.insertOrder(low);
    book.insertOrder(high);

    EXPECT_EQ(book.getBestBid().price, 105);
}

TEST(OrderBookTest, BestAskIsLowestPrice) {
    OrderBook book;
    Order high{1, 1, 105, 10, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::GTC};
    Order low{2, 2, 100, 10, Order::Side::Sell, Order::Type::Limit, Order::TimeInForce::GTC};

    book.insertOrder(high);
    book.insertOrder(low);

    EXPECT_EQ(book.getBestAsk().price, 100);
}