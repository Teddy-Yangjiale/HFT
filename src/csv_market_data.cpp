#include "hft/market_data/csv_market_data.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace hft {

namespace {

auto parse_side(const std::string& value) -> Side {
    if (value == "B" || value == "BUY") {
        return Side::Buy;
    }
    if (value == "S" || value == "SELL") {
        return Side::Sell;
    }
    throw std::runtime_error("invalid side: " + value);
}

} // namespace

auto CsvMarketDataReader::read(const std::filesystem::path& path) const -> std::vector<MarketEvent> {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open market data file: " + path.string());
    }

    std::vector<MarketEvent> events;
    std::string line;
    std::getline(input, line);

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream row(line);
        std::string field;
        MarketEvent event;

        std::getline(row, field, ',');
        event.sequence = static_cast<std::uint64_t>(std::stoull(field));

        std::getline(row, field, ',');
        event.exchange_ts_ns = static_cast<TimestampNs>(std::stoull(field));

        std::getline(row, event.symbol, ',');

        std::getline(row, field, ',');
        event.side = parse_side(field);

        std::getline(row, field, ',');
        event.price = static_cast<Price>(std::stoll(field));

        std::getline(row, field, ',');
        event.quantity = static_cast<Quantity>(std::stoll(field));

        event.type = MarketEventType::BookUpdate;
        events.push_back(std::move(event));
    }

    std::ranges::sort(events, {}, &MarketEvent::sequence);
    return events;
}

} // namespace hft
