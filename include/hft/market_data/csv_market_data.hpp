#pragma once

#include "hft/core/market_event.hpp"

#include <filesystem>
#include <vector>

namespace hft {

class CsvMarketDataReader {
public:
    [[nodiscard]] auto read(const std::filesystem::path& path) const -> std::vector<MarketEvent>;
};

} // namespace hft
