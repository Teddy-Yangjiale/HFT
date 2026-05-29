#include "hft/oms/order_manager.hpp"

namespace hft {

namespace {

auto is_active(OrderStatus status) -> bool {
    return status == OrderStatus::Accepted || status == OrderStatus::PartiallyFilled;
}

} // namespace

OrderManager::OrderManager(RiskEngine& risk)
    : risk_(risk) {}

auto OrderManager::submit(const OrderRequest& request) -> Order {
    Order order;
    order.id = next_id_++;
    order.symbol = request.symbol;
    order.side = request.side;
    order.type = request.type;
    order.price = request.price;
    order.quantity = request.quantity;

    const auto decision = risk_.check(request);
    order.status = decision.accepted ? OrderStatus::Accepted : OrderStatus::Rejected;
    orders_.emplace(order.id, order);
    return order;
}

void OrderManager::accept(OrderId id) {
    if (auto found = orders_.find(id); found != orders_.end()) {
        found->second.status = OrderStatus::Accepted;
    }
}

void OrderManager::reject(OrderId id) {
    if (auto found = orders_.find(id); found != orders_.end()) {
        if (is_active(found->second.status)) {
            risk_.release_open_order(found->second);
        }
        found->second.status = OrderStatus::Rejected;
    }
}

auto OrderManager::cancel(OrderId id) -> bool {
    auto found = orders_.find(id);
    if (found == orders_.end() || !is_active(found->second.status)) {
        return false;
    }

    risk_.release_open_order(found->second);
    found->second.status = OrderStatus::Cancelled;
    return true;
}

auto OrderManager::replace(OrderId id, const OrderRequest& replacement) -> std::optional<Order> {
    if (!cancel(id)) {
        return std::nullopt;
    }

    return submit(replacement);
}

void OrderManager::fill(const Fill& fill_event) {
    auto found = orders_.find(fill_event.order_id);
    if (found == orders_.end() || !is_active(found->second.status)) {
        return;
    }

    if (!fill_event.execution_id.empty()) {
        const auto [_, inserted] = seen_execution_ids_.insert(fill_event.execution_id);
        if (!inserted) {
            return;
        }
    }

    auto& order = found->second;
    const auto remaining = order.quantity > order.filled ? order.quantity - order.filled : 0;
    const auto fill_qty = fill_event.quantity > remaining ? remaining : fill_event.quantity;
    if (fill_qty <= 0) {
        return;
    }

    Fill adjusted_fill = fill_event;
    adjusted_fill.quantity = fill_qty;
    order.filled += fill_qty;
    order.status = order.filled >= order.quantity ? OrderStatus::Filled : OrderStatus::PartiallyFilled;
    risk_.on_fill(adjusted_fill);
}

auto OrderManager::get(OrderId id) const -> std::optional<Order> {
    const auto found = orders_.find(id);
    if (found == orders_.end()) {
        return std::nullopt;
    }
    return found->second;
}

auto OrderManager::order_count() const -> std::size_t {
    return orders_.size();
}

void OrderManager::reserve_orders(std::size_t count) {
    orders_.reserve(count);
}

} // namespace hft
