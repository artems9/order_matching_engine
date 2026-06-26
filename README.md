# Low-Latency Matching Engine

## What it is

A price-time priority order matching engine fed with live order flow
from the Binance BTC/USDT order book.

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

## Prerequisites

- C++20 compiler
- CMake 3.20+
- Python 3.12+
- websockets: pip3 install websockets

## Building

    cmake -DCMAKE_BUILD_TYPE=Release -S . -B cmake-build-release
    cmake --build cmake-build-release

## Running

    python3 -u scripts/binance_feed.py | ./cmake-build-release/trading_engine

## Running tests

    ./cmake-build-release/tests

## Running benchmark

    ./cmake-build-release/benchmark

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
median of 5 runs, release build on MacBook Air M2:

| Configuration    | Throughput        | p50   | p95   | p99    |
|------------------|-------------------|-------|-------|--------|
| Single-threaded  | ~11.5M orders/sec | 83 ns | 84 ns | 166 ns |
| Thread pool (4t) | ~4M orders/sec    | —     | —     | —      |

Single-threaded is ~3x faster — matching is inherently serial and the
thread pool adds contention overhead with no parallelism benefit.

p50 and p95 are nearly identical, indicating consistent latency with
variance only at the extreme tail.

## Profiling

Profiled with Instruments Time Profiler on MacBook Air M2.

Primary bottleneck: heap allocation.
std::list allocates a node per order insertion and frees it on removal.
At 1M orders this results in ~2M heap operations — malloc/free calls are
visible throughout the profile in insertOrder and removeBestAsk.

The standard HFT fix is a pool allocator — pre-allocate a block of
Order objects at startup and reuse them, eliminating per-order heap
allocation entirely. Real HFT systems pre-allocate all memory before
trading hours and never call malloc on the critical path.

## What's next

- Pool allocator to eliminate per-order heap allocation
- Per-instrument thread partitioning to make parallelism meaningful
- Lock-free inbound queue
- Feed Handler: parse real exchange data, maintain local order book, connect to engine

A UI?
Left panel:

* order book (bid/ask ladder)

Center:

* trades stream

Right:

* queue size + latency stats

Bottom:

* price chart (simple line)

