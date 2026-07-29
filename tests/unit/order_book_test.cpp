#include <gtest/gtest.h>
#include "order_engine/core/order_book.hpp"

using namespace order_engine;

// Helper — builds a limit order with the fields that matter for matching
static Order makeOrder(OrderId id, Side side, Price price, Quantity qty) {
    Order o;
    o.id = id;
    o.side = side;
    o.type = OrderType::Limit;
    o.price = price;
    o.initial_quantity = qty;
    o.remaining_quantity = qty;
    o.sequence_number = 0;  // book will overwrite this
    o.timestamp = std::chrono::steady_clock::now();
    return o;
}

// ========== Day 10: Exact-match crossing ==========

TEST(Matching, ExactMatch_BuyMeetsSell) {
    OrderBook book;

    // Seller rests on the book: sell 100 shares at price 100
    auto trades1 = book.addOrder(makeOrder(1, Side::Sell, 100, 100));
    EXPECT_TRUE(trades1.empty()) << "No match yet — no buyer on the book";
    EXPECT_EQ(book.getBestAsk(), 100);

    // Buyer arrives at price 100 for 100 shares — exact cross
    auto trades2 = book.addOrder(makeOrder(2, Side::Buy, 100, 100));

    // One trade produced
    ASSERT_EQ(trades2.size(), 1);
    EXPECT_EQ(trades2[0].quantity, 100);
    EXPECT_EQ(trades2[0].price, 100);
    EXPECT_EQ(trades2[0].buy_order_id, 2);
    EXPECT_EQ(trades2[0].sell_order_id, 1);

    // Book is empty — both sides fully filled
    EXPECT_EQ(book.getBestBid(), std::nullopt);
    EXPECT_EQ(book.getBestAsk(), std::nullopt);
}

TEST(Matching, ExactMatch_SellMeetsBuy) {
    OrderBook book;

    // Buyer rests on the book: buy 50 shares at price 200
    auto trades1 = book.addOrder(makeOrder(1, Side::Buy, 200, 50));
    EXPECT_TRUE(trades1.empty());
    EXPECT_EQ(book.getBestBid(), 200);

    // Seller arrives at price 200 for 50 shares — exact cross
    auto trades2 = book.addOrder(makeOrder(2, Side::Sell, 200, 50));

    ASSERT_EQ(trades2.size(), 1);
    EXPECT_EQ(trades2[0].quantity, 50);
    EXPECT_EQ(trades2[0].price, 200);
    EXPECT_EQ(trades2[0].buy_order_id, 1);
    EXPECT_EQ(trades2[0].sell_order_id, 2);

    EXPECT_EQ(book.getBestBid(), std::nullopt);
    EXPECT_EQ(book.getBestAsk(), std::nullopt);
}

TEST(Matching, NoCross_PricesDontMeet) {
    OrderBook book;

    // Seller at 100, buyer only willing to pay 95 — no cross
    book.addOrder(makeOrder(1, Side::Sell, 100, 50));
    auto trades = book.addOrder(makeOrder(2, Side::Buy, 95, 50));

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.getBestAsk(), 100);
    EXPECT_EQ(book.getBestBid(), 95);
}

// ========== Day 11: Partial fills ==========

TEST(Matching, PartialFill_BuyerSmallerThanSeller) {
    OrderBook book;

    // Seller rests with 100 shares at price 100
    book.addOrder(makeOrder(1, Side::Sell, 100, 100));

    // Buyer arrives for only 60 shares at price 100
    auto trades = book.addOrder(makeOrder(2, Side::Buy, 100, 60));

    // One trade for 60
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 60);
    EXPECT_EQ(trades[0].price, 100);

    // Buyer fully consumed — no bid left
    EXPECT_EQ(book.getBestBid(), std::nullopt);

    // Seller still has 40 remaining
    EXPECT_EQ(book.getBestAsk(), 100);
    auto snap = book.getSnapshot();
    ASSERT_EQ(snap.asks.size(), 1);
    EXPECT_EQ(snap.asks[0].total_quantity, 40);
}

TEST(Matching, PartialFill_BuyerLargerThanSeller) {
    OrderBook book;

    // Seller rests with 60 shares at price 100
    book.addOrder(makeOrder(1, Side::Sell, 100, 60));

    // Buyer arrives for 100 shares at price 100
    auto trades = book.addOrder(makeOrder(2, Side::Buy, 100, 100));

    // One trade for 60
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 60);

    // Seller fully consumed — no ask left
    EXPECT_EQ(book.getBestAsk(), std::nullopt);

    // Buyer has 40 remaining, rests on the book
    EXPECT_EQ(book.getBestBid(), 100);
    auto snap = book.getSnapshot();
    ASSERT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.bids[0].total_quantity, 40);
}

TEST(Matching, PartialFill_WalksThroughMultipleLevels) {
    OrderBook book;

    // Two sellers at different prices
    book.addOrder(makeOrder(1, Side::Sell, 100, 30));  // cheapest
    book.addOrder(makeOrder(2, Side::Sell, 101, 50));  // next level

    // Buyer wants 60 shares and is willing to pay up to 101
    auto trades = book.addOrder(makeOrder(3, Side::Buy, 101, 60));

    // First eats all 30 at price 100, then 30 at price 101
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].quantity, 30);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[1].quantity, 30);
    EXPECT_EQ(trades[1].price, 101);

    // Buyer fully filled, seller at 101 has 20 left
    EXPECT_EQ(book.getBestBid(), std::nullopt);
    EXPECT_EQ(book.getBestAsk(), 101);
    auto snap = book.getSnapshot();
    ASSERT_EQ(snap.asks.size(), 1);
    EXPECT_EQ(snap.asks[0].total_quantity, 20);
}