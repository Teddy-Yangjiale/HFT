#pragma once

#include "hft/core/types.hpp"

#include <string>

namespace hft {

enum class MarketEventType {
    BookUpdate,
    Trade
};

struct MarketEvent {
    MarketEventType type{MarketEventType::BookUpdate};
    std::string symbol;
    Side side{Side::Buy};
    Price price{0};
    Quantity quantity{0};
    TimestampNs exchange_ts_ns{0};
    std::uint64_t sequence{0};
};

} // namespace hft
