#pragma once

#include "hft/core/types.hpp"

#include <string>

namespace hft {

enum class MarketEventType {
    BookUpdate,
    Trade
};

// MarketEvent is the normalized boundary between raw exchange feeds and the rest
// of the trading system. Exchange-specific adapters should translate native
// packets into this shape, so strategies and risk logic do not depend on feed
// formats such as ITCH, FIX/FAST, WebSocket JSON, or a vendor SDK.
struct MarketEvent {
    MarketEventType type{MarketEventType::BookUpdate};
    std::string symbol;
    Side side{Side::Buy};
    Price price{0};
    Quantity quantity{0};
    // exchange_ts_ns is the timestamp supplied by the venue or data provider.
    // Later versions should also add NIC/kernel receive timestamp and app
    // receive timestamp, because latency debugging needs all three clocks.
    TimestampNs exchange_ts_ns{0};
    // sequence is used to detect gaps and to make replay deterministic. Live
    // adapters should reject or resync the book when a gap is detected.
    std::uint64_t sequence{0};
};

} // namespace hft
