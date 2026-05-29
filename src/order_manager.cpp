#include "hft/oms/order_manager.hpp"

namespace hft {

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
        if (found->second.status == OrderStatus::Accepted || found->second.status == OrderStatus::PartiallyFilled) {
            risk_.release_open_order(found->second);
        }
        found->second.status = OrderStatus::Rejected;
    }
}

void OrderManager::fill(const Fill& fill_event) {
    if (auto found = orders_.find(fill_event.order_id); found != orders_.end()) {
        auto& order = found->second;
        order.filled += fill_event.quantity;
        order.status = order.filled >= order.quantity ? OrderStatus::Filled : OrderStatus::PartiallyFilled;
        risk_.on_fill(fill_event);
    }
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
