#pragma once

#include "hft/core/market_event.hpp"
#include "hft/core/types.hpp"

#include <map>
#include <optional>
#include <string>
#include <functional>

namespace hft {

struct TopOfBook {
    std::optional<Price> bid_price;
    Quantity bid_quantity{0};
    std::optional<Price> ask_price;
    Quantity ask_quantity{0};
};

class OrderBook {
public:
    explicit OrderBook(std::string symbol);

    void apply(const MarketEvent& event);
    [[nodiscard]] auto top() const -> TopOfBook;
    [[nodiscard]] auto symbol() const -> const std::string&;

private:
    std::string symbol_;
    std::map<Price, Quantity, std::greater<>> bids_;
    std::map<Price, Quantity> asks_;
};

} // namespace hft
