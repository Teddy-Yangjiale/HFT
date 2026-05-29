#include "hft/risk/risk_engine.hpp"

#include <cmath>

namespace hft {

RiskEngine::RiskEngine(RiskLimits limits)
    : limits_(limits) {}

auto RiskEngine::check(const OrderRequest& request) -> RiskDecision {
    if (global_kill_switch_) {
        return {false, "global kill switch enabled"};
    }

    auto& state = symbol_state_[request.symbol];
    if (state.kill_switch) {
        return {false, "symbol kill switch enabled"};
    }

    if (request.quantity <= 0) {
        return {false, "quantity must be positive"};
    }
    if (request.quantity > limits_.max_single_order_qty) {
        return {false, "single order quantity limit exceeded"};
    }
    if (request.price <= 0 || request.price > limits_.max_price) {
        return {false, "price limit exceeded"};
    }

    const auto signed_qty = request.side == Side::Buy ? request.quantity : -request.quantity;
    if (std::llabs(state.position + signed_qty) > limits_.max_symbol_position) {
        return {false, "symbol position limit exceeded"};
    }

    const auto projected_open_buy = state.open_buy_qty + (request.side == Side::Buy ? request.quantity : 0);
    const auto projected_open_sell = state.open_sell_qty + (request.side == Side::Sell ? request.quantity : 0);
    if (projected_open_buy > limits_.max_open_order_qty || projected_open_sell > limits_.max_open_order_qty) {
        return {false, "open order exposure limit exceeded"};
    }

    if (limits_.max_orders_per_window > 0 && limits_.order_rate_window_ns > 0) {
        if (state.rate_window_start_ns == 0 || request.create_ts_ns >= state.rate_window_start_ns + limits_.order_rate_window_ns) {
            state.rate_window_start_ns = request.create_ts_ns;
            state.orders_in_window = 0;
        }

        if (state.orders_in_window >= limits_.max_orders_per_window) {
            return {false, "order rate limit exceeded"};
        }
    }

    if (request.side == Side::Buy) {
        state.open_buy_qty = projected_open_buy;
    } else {
        state.open_sell_qty = projected_open_sell;
    }
    ++state.orders_in_window;
    return {true, "accepted"};
}

void RiskEngine::on_fill(const Fill& fill) {
    const auto signed_qty = fill.side == Side::Buy ? fill.quantity : -fill.quantity;
    auto& state = symbol_state_[fill.symbol];
    state.position += signed_qty;

    auto& open_qty = fill.side == Side::Buy ? state.open_buy_qty : state.open_sell_qty;
    open_qty = fill.quantity >= open_qty ? 0 : open_qty - fill.quantity;
}

void RiskEngine::release_open_order(const Order& order) {
    auto& state = symbol_state_[order.symbol];
    const auto remaining = order.quantity > order.filled ? order.quantity - order.filled : 0;
    auto& open_qty = order.side == Side::Buy ? state.open_buy_qty : state.open_sell_qty;
    open_qty = remaining >= open_qty ? 0 : open_qty - remaining;
}

void RiskEngine::set_global_kill_switch(bool enabled) {
    global_kill_switch_ = enabled;
}

void RiskEngine::set_symbol_kill_switch(const std::string& symbol, bool enabled) {
    symbol_state_[symbol].kill_switch = enabled;
}

auto RiskEngine::position(const std::string& symbol) const -> Quantity {
    return state_for(symbol).position;
}

auto RiskEngine::open_buy_quantity(const std::string& symbol) const -> Quantity {
    return state_for(symbol).open_buy_qty;
}

auto RiskEngine::open_sell_quantity(const std::string& symbol) const -> Quantity {
    return state_for(symbol).open_sell_qty;
}

auto RiskEngine::global_kill_switch_enabled() const -> bool {
    return global_kill_switch_;
}

auto RiskEngine::state_for(const std::string& symbol) const -> SymbolRiskState {
    const auto found = symbol_state_.find(symbol);
    return found == symbol_state_.end() ? SymbolRiskState{} : found->second;
}

} // namespace hft
