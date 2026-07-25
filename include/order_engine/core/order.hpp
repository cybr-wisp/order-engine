
#pragma once
#include "order_engine/core/types.hpp"
#include <chrono>

namespace order_engine {

struct Order {
    OrderId id;
    Side side;
    OrderType type;
    Price price;
    Quantity initial_quantity;
    Quantity remaining_quantity;
    std::uint64_t sequence_number; // arrival order. Lower number = arrived first = fills first at the same price.
    std::chrono::steady_clock::time_point timestamp;
};

}