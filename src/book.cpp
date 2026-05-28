#include "hft/order_book/book.hpp"

#include <stdexcept>
#include <utility>

namespace hft {

OrderBook::OrderBook(std::string symbol)
    : symbol_(std::move(symbol)) {}

void OrderBook::apply(const MarketEvent& event) {
    if (event.symbol != symbol_) {
        throw std::invalid_argument("market event symbol does not match order book");
    }

    if (event.side == Side::Buy) {
        if (event.quantity == 0) {
            bids_.erase(event.price);
        } else {
            bids_[event.price] = event.quantity;
        }
        return;
    }

    if (event.quantity == 0) {
        asks_.erase(event.price);
    } else {
        asks_[event.price] = event.quantity;
    }
}

auto OrderBook::top() const -> TopOfBook {
    TopOfBook tob;
    if (!bids_.empty()) {
        tob.bid_price = bids_.begin()->first;
        tob.bid_quantity = bids_.begin()->second;
    }
    if (!asks_.empty()) {
        tob.ask_price = asks_.begin()->first;
        tob.ask_quantity = asks_.begin()->second;
    }
    return tob;
}

auto OrderBook::symbol() const -> const std::string& {
    return symbol_;
}

} // namespace hft
