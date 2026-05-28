#pragma once

#include "hft/core/types.hpp"

#include <string>

namespace hft {

struct OrderRequest {
    std::string symbol;
    Side side{Side::Buy};
    OrderType type{OrderType::Limit};
    Price price{0};
    Quantity quantity{0};
};

struct Order {
    OrderId id{0};
    std::string symbol;
    Side side{Side::Buy};
    OrderType type{OrderType::Limit};
    Price price{0};
    Quantity quantity{0};
    Quantity filled{0};
    OrderStatus status{OrderStatus::New};
};

struct Fill {
    OrderId order_id{0};
    std::string symbol;
    Side side{Side::Buy};
    Price price{0};
    Quantity quantity{0};
    TimestampNs ts_ns{0};
};

} // namespace hft
