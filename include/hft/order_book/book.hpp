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

    // Applies an absolute quantity update at one price level. A quantity of zero
    // removes the level. This is simple enough for the MVP CSV feed; real feeds
    // often distinguish add/modify/delete and may require gap recovery snapshots.
    void apply(const MarketEvent& event);

    // Returns the best bid/ask without exposing the underlying map containers.
    // Keeping this narrow API makes it easier to replace std::map with a faster
    // fixed-depth ladder or flat array once profiling identifies the right shape.
    [[nodiscard]] auto top() const -> TopOfBook;
    [[nodiscard]] auto symbol() const -> const std::string&;

private:
    std::string symbol_;
    // std::map is deliberately chosen for correctness and readability in the
    // first milestone. It is not the final data structure for a latency-sensitive
    // production book, where price-indexed arrays or cache-aware flat maps are
    // typically better.
    std::map<Price, Quantity, std::greater<>> bids_;
    std::map<Price, Quantity> asks_;
};

} // namespace hft
