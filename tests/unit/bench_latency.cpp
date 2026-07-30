
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include "order_engine/core/order_book.hpp"

using namespace order_engine;
using Clock = std::chrono::steady_clock;

static Order makeOrder(OrderId id, Side side, OrderType type, Price price, Quantity qty) {
    Order o;
    o.id = id;
    o.side = side;
    o.type = type;
    o.price = price;
    o.initial_quantity = qty;
    o.remaining_quantity = qty;
    o.sequence_number = 0;
    o.timestamp = Clock::now();
    return o;
}

static Order makeMarketOrder(OrderId id, Side side, Quantity qty) {
    Order o;
    o.id = id;
    o.side = side;
    o.type = OrderType::Market;
    o.price = 0;
    o.initial_quantity = qty;
    o.remaining_quantity = qty;
    o.sequence_number = 0;
    o.timestamp = Clock::now();
    return o;
}

template<typename Fn>
long long timeNs(Fn&& fn) {
    auto start = Clock::now();
    fn();
    auto end = Clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

void printStats(const char* label, std::vector<long long>& times) {
    std::sort(times.begin(), times.end());
    int n = times.size();
    long long median = times[n / 2];
    long long p99 = times[(int)(n * 0.99)];
    double avg = std::accumulate(times.begin(), times.end(), 0.0) / n;
    std::cout << label << " — " << n << " ops\n";
    std::cout << "  avg:    " << (int)avg << " ns\n";
    std::cout << "  median: " << median << " ns\n";
    std::cout << "  p99:    " << p99 << " ns\n\n";
}

int main() {
    std::cout << "=== Order Engine Latency Benchmark ===\n\n";
    const int N = 10000;

    // 1. Insert with no match
    {
        std::vector<long long> times;
        times.reserve(N);
        OrderBook book;
        for (int i = 0; i < N; i++) {
            Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
            Price price = (side == Side::Buy) ? (50 - (i % 10)) : (150 + (i % 10));
            auto order = makeOrder(i + 1, side, OrderType::Limit, price, 100);
            times.push_back(timeNs([&]() { book.addOrder(order); }));
        }
        printStats("INSERT (no match)", times);
    }

    // 2. Exact match (1 level)
    {
        std::vector<long long> times;
        times.reserve(N);
        for (int i = 0; i < N; i++) {
            OrderBook book;
            book.addOrder(makeOrder(1, Side::Sell, OrderType::Limit, 100, 100));
            auto buy = makeOrder(2, Side::Buy, OrderType::Limit, 100, 100);
            times.push_back(timeNs([&]() { book.addOrder(buy); }));
        }
        printStats("MATCH (exact, 1 level)", times);
    }

    // 3. Market order sweep 10 levels
    {
        std::vector<long long> times;
        times.reserve(N);
        for (int i = 0; i < N; i++) {
            OrderBook book;
            for (int j = 0; j < 10; j++) {
                book.addOrder(makeOrder(j + 1, Side::Sell, OrderType::Limit, 100 + j, 50));
            }
            auto buy = makeMarketOrder(99, Side::Buy, 500);
            times.push_back(timeNs([&]() { book.addOrder(buy); }));
        }
        printStats("MATCH (sweep 10 levels)", times);
    }

    // 4. Cancel
    {
        std::vector<long long> times;
        times.reserve(N);
        OrderBook book;
        for (int i = 0; i < N; i++) {
            book.addOrder(makeOrder(i + 1, Side::Sell, OrderType::Limit, 100 + (i % 50), 100));
        }
        for (int i = 0; i < N; i++) {
            OrderId id = i + 1;
            times.push_back(timeNs([&]() { book.cancelOrder(id); }));
        }
        printStats("CANCEL", times);
    }

    return 0;
}