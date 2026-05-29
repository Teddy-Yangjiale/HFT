#include "hft/exchange/simulated_exchange.hpp"

#include <algorithm>

namespace hft {

auto SimulatedExchange::match(const Order& order, const OrderBook& book, TimestampNs ts_ns) const -> std::optional<Fill> {
    if (order.status != OrderStatus::Accepted) {
        return std::nullopt;
    }

    const auto top = book.top();
    if (order.side == Side::Buy && top.ask_price.has_value() && order.price >= *top.ask_price) {
        return Fill{order.id, {}, order.symbol, order.side, *top.ask_price, std::min(order.quantity, top.ask_quantity), ts_ns};
    }

    if (order.side == Side::Sell && top.bid_price.has_value() && order.price <= *top.bid_price) {
        return Fill{order.id, {}, order.symbol, order.side, *top.bid_price, std::min(order.quantity, top.bid_quantity), ts_ns};
    }

    return std::nullopt;
}

} // namespace hft
