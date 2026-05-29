#pragma once

#include "hft/core/order.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace hft {

struct RiskLimits {
    Quantity max_single_order_qty{100};
    Quantity max_symbol_position{1'000};
    Quantity max_open_order_qty{1'000};
    Price max_price{1'000'000'000};
    std::uint32_t max_orders_per_window{1'000'000};
    TimestampNs order_rate_window_ns{1'000'000'000};
};

struct RiskDecision {
    bool accepted{false};
    std::string reason;
};

class RiskEngine {
public:
    explicit RiskEngine(RiskLimits limits);

    // check() is the hard pre-trade gate and reserves accepted order exposure.
    // Every order must pass through this method before it can reach an exchange
    // gateway. Keep the method predictable: no I/O, no allocation-heavy work, and
    // no dependency on slow external state.
    [[nodiscard]] auto check(const OrderRequest& request) -> RiskDecision;

    // Position is updated from fills, while outstanding order exposure is reduced
    // by the filled quantity. This keeps live position and open-order exposure as
    // separate dimensions, which prevents a strategy from bypassing limits by
    // stacking many resting orders before any fills arrive.
    void on_fill(const Fill& fill);
    void release_open_order(const Order& order);

    void set_global_kill_switch(bool enabled);
    void set_symbol_kill_switch(const std::string& symbol, bool enabled);

    [[nodiscard]] auto position(const std::string& symbol) const -> Quantity;
    [[nodiscard]] auto open_buy_quantity(const std::string& symbol) const -> Quantity;
    [[nodiscard]] auto open_sell_quantity(const std::string& symbol) const -> Quantity;
    [[nodiscard]] auto global_kill_switch_enabled() const -> bool;

private:
    struct SymbolRiskState {
        Quantity position{0};
        Quantity open_buy_qty{0};
        Quantity open_sell_qty{0};
        TimestampNs rate_window_start_ns{0};
        std::uint32_t orders_in_window{0};
        bool kill_switch{false};
    };

    [[nodiscard]] auto state_for(const std::string& symbol) const -> SymbolRiskState;

    RiskLimits limits_;
    std::unordered_map<std::string, SymbolRiskState> symbol_state_;
    bool global_kill_switch_{false};
};

} // namespace hft
