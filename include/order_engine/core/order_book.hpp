
#pragma once 
#include <map>
#include <optional>
#include <vector>
#include "order_engine/core/price_level.hpp"
#include "order_engine/core/trade.hpp"

namespace order_engine {

    struct LevelInfo {
        Price price;
        Quantity total_quantity;
        std::size_t order_count;
    };

    struct BookSnapshot {
        std::vector<LevelInfo> bids;
        std::vector<LevelInfo> asks;
    };

    class OrderBook {
        private:
        std::map<Price, PriceLevel, std::greater<>> bid_map_; 
        std::map<Price, PriceLevel> ask_map_;
        std::uint64_t next_sequence_{0}; // Starts at zero 
        std::uint64_t next_trade_id_{0};

        public:
        // Adds an order to the book. If it crosses the opposite side, matches are
        // executed and the resulting trades are returned.
        std::vector<Trade> addOrder(Order order);

        // Returns the highest bid price, or std::nullopt if the bid side is empty.
        std::optional<Price> getBestBid() const;

        // Returns the lowest ask price, or std::nullopt if the ask side is empty.
        std::optional<Price> getBestAsk() const;

        // Returns a snapshot of the full book — aggregated qty and order count per level.
        BookSnapshot getSnapshot() const;

    };


}



