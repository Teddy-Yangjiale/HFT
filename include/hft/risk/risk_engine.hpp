#pragma once

#include "hft/core/order.hpp"

#include <string>
#include <unordered_map>

namespace hft {

struct RiskLimits {
    Quantity max_single_order_qty{100};
    Quantity max_symbol_position{1'000};
    Price max_price{1'000'000'000};
};

struct RiskDecision {
    bool accepted{false};
    std::string reason;
};

class RiskEngine {
public:
    explicit RiskEngine(RiskLimits limits);

    // check() is the hard pre-trade gate. Every order must pass through this
    // method before it can reach an exchange gateway. Keep the method predictable:
    // no I/O, no allocation-heavy work, and no dependency on slow external state.
    [[nodiscard]] auto check(const OrderRequest& request) const -> RiskDecision;

    // Position is updated only from fills, not from accepted orders. That mirrors
    // real trading systems where live exposure and outstanding order exposure are
    // separate risk dimensions. Open-order exposure should be added next.
    void on_fill(const Fill& fill);
    [[nodiscard]] auto position(const std::string& symbol) const -> Quantity;

private:
    RiskLimits limits_;
    std::unordered_map<std::string, Quantity> positions_;
};

} // namespace hft
