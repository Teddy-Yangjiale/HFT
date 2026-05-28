#include "hft/exchange/simulated_exchange.hpp"
#include "hft/market_data/csv_market_data.hpp"
#include "hft/market_data/sequence_gap_detector.hpp"
#include "hft/oms/order_manager.hpp"
#include "hft/order_book/book.hpp"
#include "hft/risk/risk_engine.hpp"
#include "hft/strategy/fixed_spread_market_maker.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    const std::filesystem::path market_data_path = argc > 1 ? argv[1] : "data/sample_market_data.csv";
    const std::string symbol = argc > 2 ? argv[2] : "BTCUSDT";

    hft::CsvMarketDataReader reader;
    auto events = reader.read(market_data_path);

    hft::OrderBook book(symbol);
    hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_price = 10'000'000'000});
    hft::OrderManager oms(risk);
    hft::SimulatedExchange exchange;
    hft::FixedSpreadMarketMaker strategy(symbol, 1, 1);
    hft::SequenceGapDetector gap_detector;

    std::size_t fills = 0;
    std::size_t gaps = 0;
    for (const auto& event : events) {
        if (const auto gap = gap_detector.observe(event)) {
            ++gaps;
            std::cerr << "sequence_gap symbol=" << gap->symbol
                      << " expected=" << gap->expected
                      << " actual=" << gap->actual << '\n';
            continue;
        }

        book.apply(event);
        for (const auto& request : strategy.on_market_event(event, book)) {
            auto order = oms.submit(request);
            if (auto fill = exchange.match(order, book, event.exchange_ts_ns)) {
                oms.fill(*fill);
                ++fills;
                std::cout << "fill order_id=" << fill->order_id
                          << " side=" << hft::side_to_string(fill->side)
                          << " price=" << fill->price
                          << " qty=" << fill->quantity << '\n';
            }
        }
    }

    const auto top = book.top();
    std::cout << "events=" << events.size()
              << " orders=" << oms.order_count()
              << " fills=" << fills
              << " gaps=" << gaps
              << " position=" << risk.position(symbol);

    if (top.bid_price && top.ask_price) {
        std::cout << " top_bid=" << *top.bid_price << " top_ask=" << *top.ask_price;
    }
    std::cout << '\n';

    return 0;
}
