#pragma once

#include "hft/core/market_event.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace hft {

struct SequenceGap {
    std::string symbol;
    std::uint64_t expected{0};
    std::uint64_t actual{0};
};

class SequenceGapDetector {
public:
    // observe() enforces strictly increasing, contiguous sequence numbers per
    // detector instance. Most real feeds require this check before mutating the
    // book; once a gap is seen, the correct action is usually to stop trading the
    // symbol and recover from a clean snapshot.
    [[nodiscard]] auto observe(const MarketEvent& event) -> std::optional<SequenceGap>;

    [[nodiscard]] auto next_expected() const -> std::uint64_t;
    void reset();

private:
    std::uint64_t next_expected_{0};
};

} // namespace hft
