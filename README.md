### What it is
A C++ order matching engine that matches buy and sell orders by price priority, supports partial fills.

### How it works 
Orders are manually added. Order can be a buy or sell, have a price and a certain amount to be filled. These orders are organized into maxheap for buys and minheap for sells. The orders are then matched against eachother and a trade occurs.

### How to run it
clang++ -std=c++20 main.cpp -o prog && ./prog
