#pragma once

#include "hft/core/market_event.hpp"
#include "hft/core/types.hpp"

#include <optional>
#include <string>
#include <vector>

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
    // Keeping this narrow API made it possible to replace the original tree book
    // with a flat ladder without touching strategies or simulators.
    [[nodiscard]] auto top() const -> TopOfBook;
    [[nodiscard]] auto symbol() const -> const std::string&;

private:
    struct PriceLevel {
        Price price{0};
        Quantity quantity{0};
    };

    using Levels = std::vector<PriceLevel>;

    static void apply_level(Levels& levels, Price price, Quantity quantity, bool bid_side);

    std::string symbol_;

    // Levels are stored in contiguous vectors: bids sorted high-to-low, asks
    // sorted low-to-high. This avoids tree-node allocation and gives top() a
    // cache-friendly front() read. For very deep books, a fixed price-indexed
    // ladder can replace this without changing the public API.
    Levels bids_;
    Levels asks_;
};

} // namespace hft
