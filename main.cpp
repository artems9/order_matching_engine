#include <iostream>
#include <queue>
#include <vector>

// Represents a request to buy/sell
struct Order {
    enum class Side {
        Buy,
        Sell
    };

    enum class Type {
        Market,
        Limit
    };

    int id;
    int price;
    int quantity;

    Side side;
    Type type;
};

// Represents an executed transaction
struct Trade {
    int buyOrderID;
    int sellOrderID;

    int price;
    int quantity;
};

// maxheap
struct CompareBuys {
    bool operator() (const Order& a, const Order& b) const { return a.price < b.price; }
};

// minheap
struct CompareSells {
    bool operator() (const Order& a, const Order& b) const { return a.price > b.price; }
};

// Stores active unmatched orders
class OrderBook {
public:
    using BuyHeap = std::priority_queue<Order,
                                        std::vector<Order>,
                                        CompareBuys>;

    using SellHeap = std::priority_queue<Order,
                                         std::vector<Order>,
                                         CompareSells>;

    void add(const Order& order) {
        if (order.side == Order::Side::Buy) {
            buys_.push(order);
        } else {
            sells_.push(order);
        }
    }

    [[nodiscard]] bool hasBuys() const {
        return !buys_.empty();
    }

    [[nodiscard]] bool hasSells() const {
        return !sells_.empty();
    }

    [[nodiscard]] Order topBuy() const {
        return buys_.top();
    }
    [[nodiscard]] Order topSell() const {
        return sells_.top();
    }

    void popBuy() { buys_.pop(); }

    void popSell() { sells_.pop(); }

private:
    BuyHeap buys_;
    SellHeap sells_;
};


// Makes decisions and performs matching
class MatchingEngine {
public:
    std::vector<Trade> process(Order incomingOrder) {
        std::vector<Trade> trades;

        if (incomingOrder.side == Order::Side::Buy) {
            processBuy(incomingOrder, trades);
        } else {
            processSell(incomingOrder, trades);
        }

        return trades;
    }

private:
    OrderBook book_;

    void processBuy(Order& buyOrder, std::vector<Trade>& trades) {
        while (book_.hasSells()) {
            Order bestSell = book_.topSell();

            if (!canMatch(buyOrder, bestSell)) { break; }

            int const quantityTraded = std::min(buyOrder.quantity, bestSell.quantity);
            trades.push_back(
                Trade{
                    buyOrder.id,
                    bestSell.id,
                    bestSell.price,
                    quantityTraded
                });
            buyOrder.quantity -= quantityTraded;
            bestSell.quantity -= quantityTraded;

            book_.popSell();
            if (bestSell.quantity > 0 ) { book_.add(bestSell); }
            if (buyOrder.quantity == 0) { return; }
        }
        // Whatever is left of this order could not be matched, so it stays in the book.
        book_.add(buyOrder);
    }

    void processSell(Order& sellOrder, std::vector<Trade>& trades) {
        while (book_.hasBuys()) {
            Order bestBuy = book_.topBuy();

            if (!canMatch(bestBuy, sellOrder)) { break; }

            const int quantityTraded = std::min(bestBuy.quantity, sellOrder.quantity);

            trades.push_back(
                Trade{
                    bestBuy.id,
                    sellOrder.id,
                    bestBuy.price,
                    quantityTraded
                }
            );

            bestBuy.quantity -= quantityTraded;
            sellOrder.quantity -= quantityTraded;

            book_.popBuy();
            if (bestBuy.quantity > 0) { book_.add(bestBuy); }
            if (sellOrder.quantity == 0) { return; }
        }
        book_.add(sellOrder);
    }

    static bool canMatch(const Order& buyOrder, const Order& sellOrder) {
        return buyOrder.price >= sellOrder.price;
    }
};


int main() {
    MatchingEngine engine;
    Order buyOrder{
    1,
    105,
    100,
    Order::Side::Buy,
    Order::Type::Limit
        };

    Order sellOrder{
        2,
        103,
        50,
        Order::Side::Sell,
        Order::Type::Limit
            };

    auto trades = engine.process(buyOrder);
    for (const auto& trade : trades) {
        std::cout
            << "TRADE: "
            << trade.quantity
            << " @ "
            << trade.price
            << '\n';
    }

    std::cout << "Buy ran" << std::endl;

    trades = engine.process(sellOrder);
    for (const auto& trade : trades) {
        std::cout
            << "TRADE: "
            << trade.quantity
            << " @ "
            << trade.price
            << '\n';
    }

    return 0;
}