#include "hft/market_data/sequence_gap_detector.hpp"

namespace hft {

auto SequenceGapDetector::observe(const MarketEvent& event) -> std::optional<SequenceGap> {
    if (next_expected_ == 0) {
        next_expected_ = event.sequence + 1;
        return std::nullopt;
    }

    if (event.sequence != next_expected_) {
        const SequenceGap gap{
            .symbol = event.symbol,
            .expected = next_expected_,
            .actual = event.sequence,
        };
        next_expected_ = event.sequence + 1;
        return gap;
    }

    ++next_expected_;
    return std::nullopt;
}

auto SequenceGapDetector::next_expected() const -> std::uint64_t {
    return next_expected_;
}

void SequenceGapDetector::reset() {
    next_expected_ = 0;
}

} // namespace hft
