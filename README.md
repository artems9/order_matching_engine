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
OrderBook stores resting orders order in a std::map of price levels, 
each holding a FIFO std::list of orders. MatchingEngine owns the book
and processes incoming orders, matching them against resting orders
and returning any resulting trades and displaying them in terminal.

## Design decisions

**Why std::list over std::deque for order storage**
std::list was used to allow for fast order cancellation. 
Cancelling an order in the middle of std::deque would shift all elements
Essentially this is the difference between having O(n) vs O(1) time complexity.

**Why std::map for price levels instead of a heap**
std::map keeps price levels sorted and supports deletion and 
iteration naturally. A heap only provides efficient access to the 
top element, therefore removing arbitrary price levels or iterating in 
order requires O(n) work. std::map lookup is O(log n ) but that
tradeoff is acceptable given the other benefits. 

**Why a separate orderIdToIterator_ index**
This is a secodary/helper data structure to the orderbook map
in order to allow finding order price level in map in O(1) time instead of searching all levels

## Benchmarks
10,000 orders processed across 3 runs on MacBook M2:

| Run | Total | Avg/order |
|-----|-------|-----------|
| 1   | 3.7ms | 0.37 µs   |
| 2   | 4.7ms | 0.47 µs   |
| 3   | 4.0ms | 0.40 µs   |

Average: ~0.41 µs/order

## What I'd improve next
- Concurrency (thread-safe order ingestion)
- WebSocket feed for live market data
- Nanosecond latency benchmarking