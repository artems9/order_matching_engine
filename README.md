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
                       intrusive linked list per level
                       pool-allocated Order objects
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

    python3 -u scripts/binance_feed.py | ./cmake-build-release/main

## Running tests

    ./cmake-build-release/test

## Running benchmark

    ./cmake-build-release/benchmark

## How it works

OrderBook stores resting orders in a std::map of price levels. Each price
level holds an intrusive doubly linked list of Order objects allocated from
a fixed-size pool. MatchingEngine owns the book and processes incoming
orders, matching them against resting liquidity using price-time priority
and returning any resulting trades.

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

**Why a pool allocator instead of std::list**

std::list allocates a heap node per order insertion and frees it on removal.
At high throughput this results in continuous malloc/free calls on the
critical path. The pool allocator pre-allocates 65536 Order slots at startup
and reuses them, eliminating per-order heap allocation entirely. Real HFT
systems pre-allocate all memory before trading hours and never call malloc
on the hot path.

**Why an intrusive linked list instead of std::list**

std::list manages its own node memory separately from the stored objects.
An intrusive list embeds prev/next pointers directly in Order, so list nodes
live in the pool alongside the Order data. This eliminates a second layer of
allocation and improves cache locality when traversing a price level.

**Why O(1) cancellation**

orderPositionById_ maps order ID to a direct Order pointer. Cancel finds
the order in O(1), unlinks it from its PriceLevel in O(1) via the intrusive
prev/next pointers, destructs it, and returns the slot to the pool.

**Why std::map for price levels instead of a heap**

std::map keeps price levels sorted and supports deletion and iteration
naturally. A heap only gives efficient access to the top element —
removing arbitrary levels or iterating in order is O(n).

**Why the thread pool is slower than single-threaded**

The engine mutex serialises all matching — threads contend on one lock
with no parallelism benefit. The correct architecture is one matching
thread per instrument with a lock-free inbound queue. These benchmarks
demonstrate that tradeoff directly.

## Benchmarks

1,000,000 orders, alternating buys at 100 and sells at 99 (always crossing),
median of 5 runs, release build on MacBook Air M2:

| Configuration        | Throughput (orders/sec) | p50   | p95   | p99       |
|----------------------|-------------------------|-------|-------|-----------|
| std::list (baseline) | ~12.4M                  | 42 ns | 84 ns | 167 ns    |
| Pool allocator       | ~14.6M – 14.9M          | 42 ns | 84 ns | 84–125 ns |
| Thread pool (4t)     | ~4.0M                   | —     | —     | —         |

Pool allocator reduces p99 tail latency by ~30% by eliminating per-order
heap allocation on the hot path.

Single-threaded is ~3x faster than the thread pool — matching is inherently
serial and the thread pool adds contention overhead with no parallelism benefit.

## Profiling

Profiled with Instruments Time Profiler on MacBook Air M2.

Primary bottleneck identified: heap allocation. std::list allocates a node
per order insertion and frees it on removal. At 1M orders this results in
~2M heap operations visible throughout the profile in insertOrder and
removeBestAsk. Replaced with pool allocator — see benchmarks above.

## What's next

- Per-instrument thread partitioning to make parallelism meaningful
- Lock-free inbound queue
- Feed handler: parse real exchange data, maintain local order book,
  connect to engine