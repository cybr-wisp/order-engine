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
    EXPECT_EQ(trades2[0].price, 100);        // resting order's price
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
    EXPECT_EQ(trades2[0].buy_order_id, 1);   // resting buyer
    EXPECT_EQ(trades2[0].sell_order_id, 2);   // incoming seller

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

// Helper — builds a market order (no price needed)
static Order makeMarketOrder(OrderId id, Side side, Quantity qty) {
    Order o;
    o.id = id;
    o.side = side;
    o.type = OrderType::Market;
    o.price = 0;  // irrelevant for market orders
    o.initial_quantity = qty;
    o.remaining_quantity = qty;
    o.sequence_number = 0;
    o.timestamp = std::chrono::steady_clock::now();
    return o;
}

// ========== Day 12: Market orders ==========

TEST(Matching, MarketBuy_SweepsThreeLevels) {
    OrderBook book;

    // Three sellers at different prices
    book.addOrder(makeOrder(1, Side::Sell, 100, 30));
    book.addOrder(makeOrder(2, Side::Sell, 101, 40));
    book.addOrder(makeOrder(3, Side::Sell, 105, 50));

    // Market buy for 80 — no price limit, eats cheapest first
    auto trades = book.addOrder(makeMarketOrder(4, Side::Buy, 80));

    // Sweeps: 30@100 + 40@101 + 10@105
    ASSERT_EQ(trades.size(), 3);
    EXPECT_EQ(trades[0].quantity, 30);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[1].quantity, 40);
    EXPECT_EQ(trades[1].price, 101);
    EXPECT_EQ(trades[2].quantity, 10);
    EXPECT_EQ(trades[2].price, 105);

    // Buyer fully filled, seller at 105 has 40 left
    EXPECT_EQ(book.getBestBid(), std::nullopt);
    EXPECT_EQ(book.getBestAsk(), 105);
    auto snap = book.getSnapshot();
    ASSERT_EQ(snap.asks.size(), 1);
    EXPECT_EQ(snap.asks[0].total_quantity, 40);
}

TEST(Matching, MarketSell_SweepsMultipleBids) {
    OrderBook book;

    // Two buyers resting
    book.addOrder(makeOrder(1, Side::Buy, 50, 100));
    book.addOrder(makeOrder(2, Side::Buy, 48, 60));

    // Market sell for 120 — eats highest bid first
    auto trades = book.addOrder(makeMarketOrder(3, Side::Sell, 120));

    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].quantity, 100);
    EXPECT_EQ(trades[0].price, 50);
    EXPECT_EQ(trades[1].quantity, 20);
    EXPECT_EQ(trades[1].price, 48);

    // Seller fully filled, buyer at 48 has 40 left
    EXPECT_EQ(book.getBestAsk(), std::nullopt);
    EXPECT_EQ(book.getBestBid(), 48);
}

TEST(Matching, MarketOrder_EmptyBook_NoTrades) {
    OrderBook book;

    // Market buy into empty book — nothing to match
    auto trades = book.addOrder(makeMarketOrder(1, Side::Buy, 100));

    EXPECT_TRUE(trades.empty());
    // Market order does NOT rest on the book
    EXPECT_EQ(book.getBestBid(), std::nullopt);
    EXPECT_EQ(book.getBestAsk(), std::nullopt);
}

TEST(Matching, MarketOrder_InsufficientLiquidity_PartialFillNoRest) {
    OrderBook book;

    // Only 50 shares available
    book.addOrder(makeOrder(1, Side::Sell, 100, 50));

    // Market buy wants 200 — gets 50, remaining 150 is lost
    auto trades = book.addOrder(makeMarketOrder(2, Side::Buy, 200));

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 50);

    // Book is empty — market order did NOT rest
    EXPECT_EQ(book.getBestBid(), std::nullopt);
    EXPECT_EQ(book.getBestAsk(), std::nullopt);
}