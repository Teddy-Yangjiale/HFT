#pragma once

#include "hft/core/order.hpp"
#include "hft/order_book/book.hpp"

#include <optional>

namespace hft {

class SimulatedExchange {
public:
    // The MVP fill model only fills crossing limit orders against current top of
    // book. It does not model queue priority, partial queue depletion, cancels,
    // fees, latency, or hidden liquidity. Treat its PnL output as a plumbing test,
    // not as a tradable backtest.
    [[nodiscard]] auto match(const Order& order, const OrderBook& book, TimestampNs ts_ns) const -> std::optional<Fill>;
};

} // namespace hft
