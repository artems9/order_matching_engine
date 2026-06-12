# Low-Latency Matching Engine

## What it is
A toy implementation of a price-time priority order matching engine,
fed with live order flow from the Binance BTC/USDT order book.

## Architecture

    Binance WebSocket
          |
          v
    Python feed script       (scripts/binance_feed.py)
          |  stdout
          |-------- pipe --------|
                                 |  stdin
                                 v
                          MatchingEngine      (src/MatchingEngine.cpp)
                                 |
                                 v
                            OrderBook         (src/OrderBook.cpp)
                       price-level map
                       FIFO list per level
                                 |
                                 v
                       Trades printed to terminal

## Running

    python3 -u scripts/binance_feed.py | ./cmake-build-release/trading_engine

## How it works
OrderBook stores resting orders in a std::map of price levels, each holding
a FIFO std::list of orders. MatchingEngine owns the book and processes
incoming orders, matching them against resting liquidity using price-time
priority and returning any resulting trades.

A Python script connects to the Binance WebSocket depth stream, parses
incoming bids and asks, converts prices to integers, and pipes them into
the engine via stdin.

## Order types supported
- Limit GTC — rests on book until filled or cancelled
- Limit IOC — fills what it can, discards remainder
- Limit FOK — fills completely or cancels entirely
- Market orders
- Partial fills
- Order cancellation

## Design decisions

**Why std::list over std::deque for order storage**
std::list allows O(1) cancellation via iterator. Cancelling from the middle
of a std::deque shifts all elements — O(n).

**Why std::map for price levels instead of a heap**
std::map keeps price levels sorted and supports deletion and iteration
naturally. A heap only gives efficient access to the top element —
removing arbitrary levels or iterating in order is O(n).

**Why a separate orderPositionById_ index**
A secondary unordered_map from order ID to list iterator allows O(1)
cancellation without scanning all price levels.

**Why the thread pool is slower than single-threaded**
The engine mutex serialises all matching — threads contend on one lock
with no parallelism benefit. The correct architecture is one matching
thread per instrument with a lock-free inbound queue. These benchmarks
demonstrate that tradeoff directly.

## Benchmarks
1,000,000 orders, mixed buys and sells with overlapping prices,
release build on MacBook M2:

| Configuration    | Throughput       | p50   | p95   | p99    |
|------------------|------------------|-------|-------|--------|
| Single-threaded  | ~17M orders/sec  | 42 ns | 84 ns | 166 ns |
| Thread pool (4t) | ~4M orders/sec   | —     | —     | —      |

Single-threaded is 4x faster — matching is inherently serial.

## What's next
- Per-instrument thread partitioning to make parallelism meaningful
- Lock-free inbound queue
- Profiling with perf to identify cache bottlenecks