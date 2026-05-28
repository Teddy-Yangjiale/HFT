#pragma once

#include "hft/strategy/strategy.hpp"

#include <string>

namespace hft {

class FixedSpreadMarketMaker final : public Strategy {
public:
    FixedSpreadMarketMaker(std::string symbol, Quantity order_qty, Price edge_ticks);

    void on_market_event(const MarketEvent& event, const OrderBook& book, std::vector<OrderRequest>& output) override;

private:
    std::string symbol_;
    Quantity order_qty_;
    Price edge_ticks_;
};

} // namespace hft
