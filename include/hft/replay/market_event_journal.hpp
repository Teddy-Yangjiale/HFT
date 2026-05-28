#pragma once

#include "hft/core/market_event.hpp"

#include <filesystem>
#include <vector>

namespace hft {

class MarketEventJournal {
public:
    // The journal stores normalized MarketEvent records, not raw exchange packets.
    // This makes deterministic replay easy today. A production recorder should
    // also persist raw packets so adapter bugs can be fixed and replayed later.
    void write(const std::filesystem::path& path, const std::vector<MarketEvent>& events) const;

    // read() validates the file magic/version and returns events in journal order.
    // Callers that need gap safety should run SequenceGapDetector during replay.
    [[nodiscard]] auto read(const std::filesystem::path& path) const -> std::vector<MarketEvent>;
};

} // namespace hft
