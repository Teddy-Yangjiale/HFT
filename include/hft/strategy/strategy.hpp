#pragma once

#include "hft/core/market_event.hpp"
#include "hft/core/order.hpp"
#include "hft/order_book/book.hpp"

#include <vector>

namespace hft {

class Strategy {
public:
    virtual ~Strategy() = default;

    // Strategies consume normalized market events and the current local book,
    // then emit desired order requests. They should not know whether execution is
    // simulated, paper-traded, or live; that separation is what makes deterministic
    // replay and paper trading possible.
    [[nodiscard]] virtual auto on_market_event(const MarketEvent& event, const OrderBook& book) -> std::vector<OrderRequest> = 0;
};

} // namespace hft
