#pragma once

#include "hft/core/order.hpp"
#include "hft/order_book/book.hpp"

#include <optional>

namespace hft {

class SimulatedExchange {
public:
    [[nodiscard]] auto match(const Order& order, const OrderBook& book, TimestampNs ts_ns) const -> std::optional<Fill>;
};

} // namespace hft
