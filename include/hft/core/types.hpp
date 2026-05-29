#pragma once

#include <cstdint>
#include <string>

namespace hft {

using Price = std::int64_t;
using Quantity = std::int64_t;
using OrderId = std::uint64_t;
using TimestampNs = std::uint64_t;

enum class Side {
    Buy,
    Sell
};

enum class OrderType {
    Limit,
    Market
};

enum class OrderStatus {
    New,
    Accepted,
    PartiallyFilled,
    CancelPending,
    ReplacePending,
    Filled,
    Cancelled,
    Rejected
};

inline auto side_to_string(Side side) -> const char* {
    return side == Side::Buy ? "BUY" : "SELL";
}

inline auto status_to_string(OrderStatus status) -> const char* {
    switch (status) {
    case OrderStatus::New:
        return "NEW";
    case OrderStatus::Accepted:
        return "ACCEPTED";
    case OrderStatus::PartiallyFilled:
        return "PARTIALLY_FILLED";
    case OrderStatus::CancelPending:
        return "CANCEL_PENDING";
    case OrderStatus::ReplacePending:
        return "REPLACE_PENDING";
    case OrderStatus::Filled:
        return "FILLED";
    case OrderStatus::Cancelled:
        return "CANCELLED";
    case OrderStatus::Rejected:
        return "REJECTED";
    }
    return "UNKNOWN";
}

} // namespace hft
