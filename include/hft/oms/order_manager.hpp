#pragma once

#include "hft/core/order.hpp"
#include "hft/risk/risk_engine.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace hft {

class OrderManager {
public:
    explicit OrderManager(RiskEngine& risk);

    // submit() performs local validation through RiskEngine and creates the OMS
    // order record. In production, accepted orders would then be handed to a
    // gateway, and exchange ack/reject callbacks would drive the final state.
    [[nodiscard]] auto submit(const OrderRequest& request) -> Order;
    void accept(OrderId id);
    void reject(OrderId id);
    [[nodiscard]] auto cancel(OrderId id) -> bool;
    [[nodiscard]] auto replace(OrderId id, const OrderRequest& replacement) -> std::optional<Order>;

    // fill() uses execution_id for idempotency when the venue supplies one. Empty
    // execution IDs are accepted for simulator compatibility but cannot be
    // deduplicated, so live gateways should always populate the field.
    void fill(const Fill& fill);

    [[nodiscard]] auto get(OrderId id) const -> std::optional<Order>;
    [[nodiscard]] auto order_count() const -> std::size_t;
    void reserve_orders(std::size_t count);

private:
    RiskEngine& risk_;
    OrderId next_id_{1};
    std::unordered_map<OrderId, Order> orders_;
    std::unordered_set<std::string> seen_execution_ids_;
};

} // namespace hft
