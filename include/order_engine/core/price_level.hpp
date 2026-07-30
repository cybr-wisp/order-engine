
#pragma once
#include "order_engine/core/order.hpp"
#include <list>
#include <cstddef>

namespace order_engine {
    struct PriceLevel {
        Price price;

        // FIFO queue:
        // front = arrived first = fills first
        std::list<Order> orders;

        // While this function runs, it is not allowed to modify the PriceLevel object
        Quantity totalQuantity() const {
            Quantity total = 0;

            // For each Order inside the orders list, temporarily call it order, 
            // refer to the original object without copying it, and do not modify it
            for (const Order& order : orders) {
                total += order.remaining_quantity;
            }

            return total;
        }


        std::size_t orderCount() const {
        return orders.size();
        }

        bool empty() const {
            return orders.empty();
        }

        



    };
}
