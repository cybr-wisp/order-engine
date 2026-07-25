#pragma once
#include "order_engine/core/types.hpp"
#include <cstdint>
#include <chrono>

namespace order_engine {

struct Trade {
    std::uint64_t trade_id;
    OrderId buy_order_id;
    OrderId sell_order_id;
    Price price;
    Quantity quantity;
    std::chrono::steady_clock::time_point timestamp;
};

}