#pragma once

#include "hft/core/order.hpp"
#include "hft/risk/risk_engine.hpp"

#include <optional>
#include <unordered_map>

namespace hft {

class OrderManager {
public:
    explicit OrderManager(RiskEngine& risk);

    [[nodiscard]] auto submit(const OrderRequest& request) -> Order;
    void accept(OrderId id);
    void reject(OrderId id);
    void fill(const Fill& fill);

    [[nodiscard]] auto get(OrderId id) const -> std::optional<Order>;
    [[nodiscard]] auto order_count() const -> std::size_t;

private:
    RiskEngine& risk_;
    OrderId next_id_{1};
    std::unordered_map<OrderId, Order> orders_;
};

} // namespace hft
