#pragma once

#include "hft/core/market_event.hpp"

#include <filesystem>
#include <vector>

namespace hft {

class CsvMarketDataReader {
public:
    // CSV replay is intentionally boring and deterministic. It gives us a stable
    // local feed for tests and benchmarks before we add venue-specific live
    // adapters, packet gap handling, and snapshot recovery.
    [[nodiscard]] auto read(const std::filesystem::path& path) const -> std::vector<MarketEvent>;
};

} // namespace hft
