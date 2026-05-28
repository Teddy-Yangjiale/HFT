#include "hft/risk/risk_engine.hpp"

#include <cmath>

namespace hft {

RiskEngine::RiskEngine(RiskLimits limits)
    : limits_(limits) {}

auto RiskEngine::check(const OrderRequest& request) const -> RiskDecision {
    if (request.quantity <= 0) {
        return {false, "quantity must be positive"};
    }
    if (request.quantity > limits_.max_single_order_qty) {
        return {false, "single order quantity limit exceeded"};
    }
    if (request.price <= 0 || request.price > limits_.max_price) {
        return {false, "price limit exceeded"};
    }

    const auto current_position = position(request.symbol);
    const auto signed_qty = request.side == Side::Buy ? request.quantity : -request.quantity;
    if (std::llabs(current_position + signed_qty) > limits_.max_symbol_position) {
        return {false, "symbol position limit exceeded"};
    }

    return {true, "accepted"};
}

void RiskEngine::on_fill(const Fill& fill) {
    const auto signed_qty = fill.side == Side::Buy ? fill.quantity : -fill.quantity;
    positions_[fill.symbol] += signed_qty;
}

auto RiskEngine::position(const std::string& symbol) const -> Quantity {
    const auto found = positions_.find(symbol);
    return found == positions_.end() ? 0 : found->second;
}

} // namespace hft
