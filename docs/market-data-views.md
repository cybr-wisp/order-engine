
# Market Data Views: MBO and MBP

## Overview

This project implements two complementary views of the order book: **Market by Order (MBO)** and **Market by Price (MBP)**. The engine is MBO internally — it stores every individual order in FIFO queues and derives MBP snapshots as an aggregated external view.

---

## Market by Order (MBO)

Market by Order is a high-granularity market data feed (often called Level 3 data) that exposes every individual order in the book.

### Key Features

- **Individual Orders:** Lists every single buy and sell order separately instead of grouping them by price.
- **Queue Position:** Shows exactly where an individual order sits in line at a specific price level.
- **Real-Time Updates:** Tracks exact changes, additions, and cancellations of specific orders as they happen.
- **Full Depth of Book:** Displays all active orders across all available price levels.

### What MBO Looks Like

At price level $100.20, the MBO view shows each resting order individually:

```
Price $100.20 — 3 orders, FIFO queue:
  Position 1:  Order 1031  qty 400  (arrived 16:41:02)
  Position 2:  Order 1035  qty 250  (arrived 16:41:08)
  Position 3:  Order 1041  qty 600  (arrived 16:41:15)
```

The matching engine walks this queue front-to-back. Order 1031 fills first, then 1035, then 1041. Queue position matters — being 1st in line at a price is fundamentally different from being 3rd.

### Sources

- [Databento — Market by Order](https://databento.com/microstructure/mbo)
- [Bookmap — CME Futures MBO Data](https://bookmap.com/blog/cme-futures-mbo-data)
- [CME Group — Market by Order FAQ](https://www.cmegroup.com/articles/faqs/market-by-order-mbo.html)

---

## Market by Price (MBP)

Market by Price is an aggregated view that collapses all orders at a given price level into a single summary row showing total quantity and order count.

### What MBP Looks Like

The same $100.20 level from above becomes one row:

```
Orders  Quantity     Price
     3    1,250   $100.20
```

MBP is what most exchange feeds publish externally (CME, Nasdaq ITCH top-of-book). It is compact, efficient to transmit, and gives a fast read on liquidity concentration — but hides individual order sizes and queue position.

### Sources

- [CME Group — Market by Order FAQ (MBO vs MBP comparison)](https://www.cmegroup.com/articles/faqs/market-by-order-mbo.html)

---

## MBO vs MBP Comparison

| Property              | Market by Order (MBO)                        | Market by Price (MBP)                          |
|-----------------------|----------------------------------------------|------------------------------------------------|
| Granularity           | Individual orders                            | Aggregated per price level                     |
| Queue position        | Visible                                      | Hidden                                         |
| Order count per level | Derivable (count the orders)                 | Shown as a summary field                       |
| Data volume           | High — every order is a row                  | Low — one row per price level                  |
| Use case              | Full book reconstruction, queue analysis     | Quick liquidity overview, dashboard display    |
| Real-world equivalent | Nasdaq TotalView, CBOE PITCH, CME MBO        | Most Level 2 feeds, top-of-book summaries      |

---

## How This Project Implements Both

### Internal Representation (MBO)

The engine stores every order individually in FIFO queues per price level:

```cpp
std::map<Price, std::list<Order>, std::greater<Price>> bids_;
std::map<Price, std::list<Order>, std::less<Price>>    asks_;
std::unordered_map<OrderId, OrderLocation>             order_lookup_;
```

Each `std::list<Order>` is a FIFO queue. The matching engine iterates front-to-back for price-time priority. This is the MBO source of truth — the engine needs individual order visibility to match correctly.

### Derived View (MBP)

MBP snapshots are computed from the MBO data by iterating each price level's queue:

```cpp
struct PriceLevelSnapshot {
    Price price;
    Quantity total_quantity;   // sum of remaining_quantity across all orders at this level
    std::size_t order_count;  // number of individual orders resting at this level
};

struct BookSnapshot {
    std::vector<PriceLevelSnapshot> bids;
    std::vector<PriceLevelSnapshot> asks;
    std::optional<Price> best_bid;
    std::optional<Price> best_ask;
    std::optional<Price> last_trade_price;
};
```

For efficiency, `total_quantity` and `order_count` can be cached as running totals on each price level and updated incrementally on add, remove, and fill — making snapshot generation O(levels) instead of O(orders).

### Where Each View Appears

| Context                        | View Used |
|--------------------------------|-----------|
| Matching engine internals      | MBO       |
| FTXUI dashboard (default)      | MBP       |
| v3.0 market data subscriptions | Both      |
| Event recording / replay       | MBO       |
| Benchmark reporting            | MBP       |

---

## Note on Market Orders vs Market by Order

These are unrelated concepts that share the word "market":

- **Market order:** A basic request to buy or sell immediately at the best available price. It has no price constraint and executes against whatever is resting in the book. ([Investopedia](https://www.investopedia.com/terms/m/marketorder.asp))
- **Market by Order (MBO):** A data feed format that shows individual orders. It describes how book data is *presented*, not how an order *behaves*.

This project supports market orders as an order type (`OrderType::Market`) and uses MBO as its internal data representation.