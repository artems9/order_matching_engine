# Trading Engine

## What it is
This project is a toy implementation of an order
matching engine containing an orderbook internally
used for processing orders placed by traders on exchanges.

## How it works
OrderBook stores orders in price-level maps with a FIFO list per level,
MatchingEngine owns the book and handles matching logic,
trades are returned as a vector.

The engine has two main components: OrderBook and MatchingEngine.
OrderBook stores resting orders in a std::map of price levels,
each holding a FIFO std::list of orders. MatchingEngine owns the book
and processes incoming orders, matching them against resting orders
and returning any resulting trades and displaying them in terminal.

A ThreadPool sits in front of the engine, accepting incoming orders via
a thread-safe queue and dispatching them to worker threads for processing.

## Design decisions

**Why std::list over std::deque for order storage**
std::list was used to allow for fast order cancellation.
Cancelling an order in the middle of std::deque would shift all elements.
Essentially this is the difference between having O(n) vs O(1) time complexity.

**Why std::map for price levels instead of a heap**
std::map keeps price levels sorted and supports deletion and
iteration naturally. A heap only provides efficient access to the
top element, therefore removing arbitrary price levels or iterating in
order requires O(n) work. std::map lookup is O(log n) but that
tradeoff is acceptable given the other benefits.

**Why a separate orderPositionById_ index**
A secondary lookup table mapping order ID to its position in the list,
allowing O(1) cancel without searching all price levels.

**Why the thread pool is slower than single-threaded**
The engine mutex serialises all matching — threads contend on one lock
rather than working in parallel. The thread pool adds queue/mutex/context-switch
overhead with no parallelism benefit. The correct architecture for a matching
engine is a single dedicated matching thread per instrument, with a lock-free
queue for ingestion. These benchmarks demonstrate that tradeoff directly.

## Benchmarks
Some definitions: 
Latency is the time it takes to complete a single operation. (Speed of one request)
Throughput is how many operations can be completed in a given time period. (Volume over time)

1,000,000 orders, mixed buys and sells with overlapping prices (real matching),
release build on MacBook M2:

| Configuration      | Total    | Avg/order | Throughput       |
|--------------------|----------|-----------|------------------|
| Single-threaded    | 58.8ms   | 0.06 µs   | ~17M orders/sec  |
| Thread pool (4t)   | 253ms    | 0.25 µs   | ~4M orders/sec   |

| Metric | Value |
|--------|-------|
| Throughput | ~17M orders/sec |
| p50 latency | 42 ns |
| p95 latency | 84 ns |
| p99 latency | 166 ns |

The single-threaded version is 4x faster because matching is inherently
serial — adding threads only adds contention overhead without parallelism benefit.

## What's next
- Binance WebSocket feed for live crypto order flow
- Python terminal dashboard showing live bids, asks, spread
- Profile with Instruments to find and fix the bottleneck
- Per-instrument thread partitioning to make parallelism meaningful