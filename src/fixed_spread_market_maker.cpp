#include "hft/strategy/fixed_spread_market_maker.hpp"

#include <utility>

namespace hft {

FixedSpreadMarketMaker::FixedSpreadMarketMaker(std::string symbol, Quantity order_qty, Price edge_ticks)
    : symbol_(std::move(symbol)),
      order_qty_(order_qty),
      edge_ticks_(edge_ticks) {}

void FixedSpreadMarketMaker::on_market_event(const MarketEvent& event, const OrderBook& book, std::vector<OrderRequest>& output) {
    if (event.symbol != symbol_) {
        return;
    }

    const auto tob = book.top();
    if (!tob.bid_price.has_value() || !tob.ask_price.has_value() || *tob.bid_price >= *tob.ask_price) {
        return;
    }

    const auto bid = *tob.bid_price + edge_ticks_;
    const auto ask = *tob.ask_price - edge_ticks_;
    if (bid >= ask) {
        return;
    }

    output.push_back(OrderRequest{symbol_, Side::Buy, OrderType::Limit, bid, order_qty_});
    output.push_back(OrderRequest{symbol_, Side::Sell, OrderType::Limit, ask, order_qty_});
}

} // namespace hft
