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

    [[nodiscard]] auto check(const OrderRequest& request) const -> RiskDecision;
    void on_fill(const Fill& fill);
    [[nodiscard]] auto position(const std::string& symbol) const -> Quantity;

private:
    RiskLimits limits_;
    std::unordered_map<std::string, Quantity> positions_;
};

} // namespace hft
