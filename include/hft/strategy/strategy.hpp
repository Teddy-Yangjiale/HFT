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
    // then append desired order requests into caller-owned storage. Caller-owned
    // output keeps the hot path allocation-aware: the event loop can reserve once
    // and reuse the same buffer for every tick instead of constructing a fresh
    // vector on each market event.
    virtual void on_market_event(const MarketEvent& event, const OrderBook& book, std::vector<OrderRequest>& output) = 0;
};

} // namespace hft
