

# Order Engine

A high-performance order book and matching engine written in modern C++20.

The engine takes buy and sell orders, organizes them by price-time priority, and executes trades when compatible orders meet — the same core system that sits at the heart of any stock exchange or trading venue.

## What It Does

- **Limit orders** — buy or sell at a specific price, rest in the book until matched
- **Market orders** — execute immediately at the best available price
- **Partial fills** — large orders fill incrementally across multiple resting orders
- **Multi-level sweeps** — market orders walk through several price levels
- **Cancellation** — remove resting orders by ID with O(1) lookup
- **Price-time priority** — best price first, earliest arrival first at the same price

## Market Data Views

The engine is **Market by Order (MBO)** internally — every individual order is stored in FIFO queues per price level. It derives **Market by Price (MBP)** snapshots for display, aggregating total quantity and order count at each level.

See [docs/market-data-views.md](docs/market-data-views.md) for details.

## Architecture

```
Interfaces (FTXUI Dashboard, Python Tooling)
        │
   Application Layer (Parser, Validator, Dispatcher)
        │
   Matching Engine (Price-Time Priority, Fills, Cancellation)
        │
   In-Memory State (Bid/Ask Maps, FIFO Queues, Order Index, Trade Log)
```

Prices are stored as integer ticks (cents) to avoid floating-point comparison errors. `$100.25` → `10025`.

## Tech Stack

- **Language:** C++20
- **Build:** CMake
- **Testing:** GoogleTest
- **Interface:** FTXUI (planned)
- **Tooling:** Python, Docker, GitHub Actions

## Build

```bash
cmake -B build -S .
cmake --build build
```

## Test

```bash
# Linux / macOS
cd build && ctest --output-on-failure

# Windows (MSVC)
cd build; ctest -C Debug --output-on-failure
```

## Release Plan

| Version | Goal | Status |
|---------|------|--------|
| v1.0 | Correct single-process engine with tests, benchmarks, FTXUI dashboard, Docker, and CI | In progress |
| v2.0 | Concurrent event-driven engine with pub-sub, metrics, and deterministic replay | Planned |
| v3.0 | Networked multi-client service with TCP protocol and market data subscriptions | Planned |

## Project Structure

```
order-engine/
├── CMakeLists.txt
├── include/order_engine/core/    # Public headers (types, order, trade)
├── src/core/                     # Engine implementation
├── tests/unit/                   # GoogleTest unit tests
├── docs/                         # Architecture and design docs
└── apps/                         # CLI and server entry points
```

## License

See [LICENSE.md](LICENSE.md).