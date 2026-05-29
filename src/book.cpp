#include "hft/order_book/book.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace hft {

namespace {

template <typename Levels>
auto find_bid_level(Levels& levels, Price price) {
    return std::lower_bound(levels.begin(), levels.end(), price, [](const auto& level, Price target) {
        return level.price > target;
    });
}

template <typename Levels>
auto find_ask_level(Levels& levels, Price price) {
    return std::lower_bound(levels.begin(), levels.end(), price, [](const auto& level, Price target) {
        return level.price < target;
    });
}

template <typename Iterator>
auto level_matches(Iterator iterator, Iterator end, Price price) -> bool {
    return iterator != end && iterator->price == price;
}

} // namespace

OrderBook::OrderBook(std::string symbol)
    : symbol_(std::move(symbol)) {
    bids_.reserve(256);
    asks_.reserve(256);
}

void OrderBook::apply(const MarketEvent& event) {
    if (event.symbol != symbol_) {
        throw std::invalid_argument("market event symbol does not match order book");
    }

    apply_level(event.side == Side::Buy ? bids_ : asks_, event.price, event.quantity, event.side == Side::Buy);
}

void OrderBook::apply_level(Levels& levels, Price price, Quantity quantity, bool bid_side) {
    const auto iterator = bid_side ? find_bid_level(levels, price) : find_ask_level(levels, price);
    if (quantity == 0) {
        if (level_matches(iterator, levels.end(), price)) {
            levels.erase(iterator);
        }
        return;
    }

    if (level_matches(iterator, levels.end(), price)) {
        iterator->quantity = quantity;
        return;
    }

    levels.insert(iterator, PriceLevel{price, quantity});
}

auto OrderBook::top() const -> TopOfBook {
    TopOfBook tob;
    if (!bids_.empty()) {
        tob.bid_price = bids_.front().price;
        tob.bid_quantity = bids_.front().quantity;
    }
    if (!asks_.empty()) {
        tob.ask_price = asks_.front().price;
        tob.ask_quantity = asks_.front().quantity;
    }
    return tob;
}

auto OrderBook::symbol() const -> const std::string& {
    return symbol_;
}

} // namespace hft
