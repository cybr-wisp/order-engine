

// orderbook.cpp is where addOrder, cancelOrder, getBestBid, getBestAsk, snapshot
#include "order_engine/core/order_book.hpp"
#include "order_engine/core/order.hpp"

namespace order_engine {

    std::vector<Trade> OrderBook::addOrder(Order order) {

        order.sequence_number = next_sequence_++;


        if (order.side == Side::Buy) {
            bid_map_[order.price].orders.push_back(order);
        } else {
            ask_map_[order.price].orders.push_back(order);
        }

        return {};  // <-- empty vector, no matching yet

    }

    std::optional<Price> OrderBook::getBestBid() const {

        if(!bid_map_.empty()) {
            return bid_map_.begin()->first;
        } else {
            return std::nullopt;
        }

    }

    std::optional<Price> OrderBook::getBestAsk() const {
        if(!ask_map_.empty()) {
            return ask_map_.begin()->first;
        } else {
            return std::nullopt;
        }

    }

    BookSnapshot OrderBook::getSnapshot() const {
        BookSnapshot snapshot;

        for (const auto& [price,level] : bid_map_) {

            LevelInfo info;
            info.price = price;
            info.total_quantity = level.totalQuantity();
            info.order_count = level.orderCount();

            snapshot.bids.push_back(info);

        }

        for (const auto& [price,level] : ask_map_) {

            LevelInfo info;
            info.price = price;
            info.total_quantity = level.totalQuantity();
            info.order_count = level.orderCount();

            snapshot.asks.push_back(info);
        }

        return snapshot;
            
    }



}






