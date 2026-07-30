

// orderbook.cpp is where addOrder, cancelOrder, getBestBid, getBestAsk, snapshot
#include "order_engine/core/order_book.hpp"
#include "order_engine/core/order.hpp"

namespace order_engine {

    // Function ->  Adds an Order to the Bid or Ask Map and returns the match 
    std::vector<Trade> OrderBook::addOrder(Order order) {
        // Stamp arrival order — used to break ties at the same price (lower = earlier = fills first)
        order.sequence_number = next_sequence_++;
        // Declare an empty vector to collect trades as we produce them 
        std::vector<Trade> trades;

        if (order.side == Side::Buy) {

            while (order.remaining_quantity > 0 && !ask_map_.empty() && (order.type == OrderType::Market || order.price >= ask_map_.begin()->first)) {
                // match against the asks, NOT Insert 
                // The seller we are matching - the existing seller in the book 
                Order& resting = ask_map_.begin()->second.orders.front();
                
                // determine how much can be matched? - only the smaller amount can be traded
                Quantity fill_qty = std::min(
                    order.remaining_quantity,
                    resting.remaining_quantity
                );

                // Update the fill_qty from both orders
                order.remaining_quantity -= fill_qty;
                resting.remaining_quantity -= fill_qty;    
                
                // Build the Trade 
                Trade trade;
                trade.trade_id = next_trade_id_++;
                trade.buy_order_id = order.id;
                trade.sell_order_id = resting.id;
                trade.price = resting.price;
                trade.quantity = fill_qty;
                trade.timestamp = std::chrono::steady_clock::now();

                trades.push_back(trade);

                // Clean up: remove filled resting order, then empty level
                if (resting.remaining_quantity == 0) {
                    ask_map_.begin()->second.orders.pop_front();
                } 
                
                if (ask_map_.begin() -> second.orders.empty() ) {
                    ask_map_.erase(ask_map_.begin());
                }

            }

        } else {
            while(order.remaining_quantity > 0 && !bid_map_.empty() && (order.type == OrderType::Market || order.price <= bid_map_.begin()->first))  {

                // match against the bids, NOT Insert 
                // The buyer we are matching - the existing buyer (resting) in the book 
                Order& resting = bid_map_.begin()->second.orders.front();
                
                // determine how much can be matched? - only the smaller amount can be traded
                Quantity fill_qty = std::min(
                    order.remaining_quantity,
                    resting.remaining_quantity
                );

                // Update the fill_qty from both orders
                order.remaining_quantity -= fill_qty;
                resting.remaining_quantity -= fill_qty;      
                
                // Build the Trade 
                Trade trade;
                trade.trade_id = next_trade_id_++;
                trade.sell_order_id = order.id;
                trade.buy_order_id = resting.id;
                trade.price = resting.price;
                trade.quantity = fill_qty;
                trade.timestamp = std::chrono::steady_clock::now();

                trades.push_back(trade);       

                // Clean up: remove filled resting order, then empty level
                if (resting.remaining_quantity == 0) {
                    bid_map_.begin()->second.orders.pop_front();
                } 
                
                if (bid_map_.begin() -> second.orders.empty() ) {
                    bid_map_.erase(bid_map_.begin());
                }
                
            }
        }

        // After the Trade - Fill the remaining 
        if (order.remaining_quantity > 0 && order.type == OrderType::Limit) {
            if (order.side == Side::Buy) {
                bid_map_[order.price].orders.push_back(order);
            } else {
                ask_map_[order.price].orders.push_back(order);
            }
        }

        return trades;
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
