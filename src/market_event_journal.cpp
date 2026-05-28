#include "hft/replay/market_event_journal.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

namespace hft {

namespace {

constexpr std::array<char, 8> kMagic{'H', 'F', 'T', 'J', 'R', 'N', 'L', '1'};
constexpr std::uint32_t kVersion = 1;

template <typename T>
void write_pod(std::ofstream& output, const T& value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof(T));
    if (!output) {
        throw std::runtime_error("failed to write journal");
    }
}

template <typename T>
auto read_pod(std::ifstream& input) -> T {
    T value{};
    input.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!input) {
        throw std::runtime_error("failed to read journal");
    }
    return value;
}

auto side_to_u8(Side side) -> std::uint8_t {
    return side == Side::Buy ? 0 : 1;
}

auto u8_to_side(std::uint8_t value) -> Side {
    if (value == 0) {
        return Side::Buy;
    }
    if (value == 1) {
        return Side::Sell;
    }
    throw std::runtime_error("invalid side in journal");
}

auto event_type_to_u8(MarketEventType type) -> std::uint8_t {
    return type == MarketEventType::BookUpdate ? 0 : 1;
}

auto u8_to_event_type(std::uint8_t value) -> MarketEventType {
    if (value == 0) {
        return MarketEventType::BookUpdate;
    }
    if (value == 1) {
        return MarketEventType::Trade;
    }
    throw std::runtime_error("invalid market event type in journal");
}

} // namespace

void MarketEventJournal::write(const std::filesystem::path& path, const std::vector<MarketEvent>& events) const {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("failed to open journal for write: " + path.string());
    }

    output.write(kMagic.data(), static_cast<std::streamsize>(kMagic.size()));
    write_pod(output, kVersion);
    write_pod(output, static_cast<std::uint64_t>(events.size()));

    for (const auto& event : events) {
        write_pod(output, event.sequence);
        write_pod(output, event.exchange_ts_ns);
        write_pod(output, event.price);
        write_pod(output, event.quantity);
        write_pod(output, event_type_to_u8(event.type));
        write_pod(output, side_to_u8(event.side));

        const auto symbol_size = static_cast<std::uint32_t>(event.symbol.size());
        write_pod(output, symbol_size);
        output.write(event.symbol.data(), symbol_size);
        if (!output) {
            throw std::runtime_error("failed to write journal symbol");
        }
    }
}

auto MarketEventJournal::read(const std::filesystem::path& path) const -> std::vector<MarketEvent> {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open journal for read: " + path.string());
    }

    std::array<char, 8> magic{};
    input.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (magic != kMagic) {
        throw std::runtime_error("invalid journal magic");
    }

    const auto version = read_pod<std::uint32_t>(input);
    if (version != kVersion) {
        throw std::runtime_error("unsupported journal version");
    }

    const auto count = read_pod<std::uint64_t>(input);
    std::vector<MarketEvent> events;
    events.reserve(static_cast<std::size_t>(count));

    for (std::uint64_t i = 0; i < count; ++i) {
        MarketEvent event;
        event.sequence = read_pod<std::uint64_t>(input);
        event.exchange_ts_ns = read_pod<TimestampNs>(input);
        event.price = read_pod<Price>(input);
        event.quantity = read_pod<Quantity>(input);
        event.type = u8_to_event_type(read_pod<std::uint8_t>(input));
        event.side = u8_to_side(read_pod<std::uint8_t>(input));

        const auto symbol_size = read_pod<std::uint32_t>(input);
        event.symbol.resize(symbol_size);
        input.read(event.symbol.data(), symbol_size);
        if (!input) {
            throw std::runtime_error("failed to read journal symbol");
        }

        events.push_back(std::move(event));
    }

    return events;
}

} // namespace hft
