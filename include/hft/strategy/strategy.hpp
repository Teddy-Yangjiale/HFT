#pragma once

#include "hft/core/market_event.hpp"
#include "hft/core/order.hpp"
#include "hft/order_book/book.hpp"

#include <vector>

namespace hft {

class Strategy {
public:
    virtual ~Strategy() = default;
    [[nodiscard]] virtual auto on_market_event(const MarketEvent& event, const OrderBook& book) -> std::vector<OrderRequest> = 0;
};

} // namespace hft
