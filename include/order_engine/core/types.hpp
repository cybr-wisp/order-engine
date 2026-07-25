

#pragma once // Only process this file once 
#include <cstdint> 

namespace order_engine {

    // Define Order-Book Variables 
    using OrderId  = std::uint64_t;
    using Price    = std::int64_t;
    using Quantity = std::uint64_t;

    enum class Side { Buy, Sell};
    enum class OrderType { Limit, Market };
    
}
