#include "hft/exchange/simulated_exchange.hpp"
#include "hft/market_data/csv_market_data.hpp"
#include "hft/market_data/sequence_gap_detector.hpp"
#include "hft/metrics/latency_stats.hpp"
#include "hft/oms/order_manager.hpp"
#include "hft/order_book/book.hpp"
#include "hft/replay/market_event_journal.hpp"
#include "hft/risk/risk_engine.hpp"
#include "hft/strategy/fixed_spread_market_maker.hpp"

#include <cassert>
#include <filesystem>
#include <vector>

namespace {

void test_order_book_top() {
    hft::OrderBook book("BTCUSDT");
    book.apply({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Buy, 100, 5, 1, 1});
    book.apply({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Buy, 101, 3, 2, 2});
    book.apply({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Sell, 103, 4, 3, 3});

    const auto top = book.top();
    assert(top.bid_price == 101);
    assert(top.bid_quantity == 3);
    assert(top.ask_price == 103);
}

void test_risk_rejects_large_order() {
    hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_price = 1'000});
    const auto decision = risk.check({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 11});
    assert(!decision.accepted);
}

void test_strategy_and_simulated_fill() {
    hft::OrderBook book("BTCUSDT");
    book.apply({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Buy, 100, 5, 1, 1});
    book.apply({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Sell, 104, 5, 2, 2});

    hft::FixedSpreadMarketMaker strategy("BTCUSDT", 1, 1);
    std::vector<hft::OrderRequest> requests;
    requests.reserve(2);
    strategy.on_market_event({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Sell, 104, 5, 2, 2}, book, requests);
    assert(requests.size() == 2);
    assert(requests[0].price == 101);
    assert(requests[1].price == 103);

    hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_price = 1'000});
    hft::OrderManager oms(risk);
    hft::SimulatedExchange exchange;

    auto crossing = oms.submit({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 105, 2});
    auto fill = exchange.match(crossing, book, 10);
    assert(fill.has_value());
    oms.fill(*fill);
    assert(risk.position("BTCUSDT") == 2);
}

void test_csv_reader() {
    hft::CsvMarketDataReader reader;
    const auto events = reader.read(std::filesystem::path("data/sample_market_data.csv"));
    assert(events.size() >= 4);
    assert(events.front().symbol == "BTCUSDT");
}

void test_latency_stats() {
    hft::LatencyStats stats;
    stats.observe(10);
    stats.observe(30);
    stats.observe(20);

    const auto summary = stats.summary();
    assert(summary.count == 3);
    assert(summary.min_ns == 10);
    assert(summary.max_ns == 30);
    assert(summary.mean_ns == 20);
    assert(summary.p50_ns == 20);
}

void test_sequence_gap_detector() {
    hft::SequenceGapDetector detector;

    const auto first = detector.observe({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Buy, 100, 1, 1, 10});
    assert(!first.has_value());

    const auto second = detector.observe({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Buy, 101, 1, 2, 11});
    assert(!second.has_value());

    const auto gap = detector.observe({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Buy, 102, 1, 3, 13});
    assert(gap.has_value());
    assert(gap->expected == 12);
    assert(gap->actual == 13);
}

void test_market_event_journal_round_trip() {
    const std::vector<hft::MarketEvent> events{
        {hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Buy, 100, 5, 1, 1},
        {hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Sell, 101, 6, 2, 2},
    };

    const auto path = std::filesystem::temp_directory_path() / "hft_market_event_journal_test.bin";
    hft::MarketEventJournal journal;
    journal.write(path, events);
    const auto restored = journal.read(path);
    std::filesystem::remove(path);

    assert(restored.size() == events.size());
    assert(restored[0].symbol == "BTCUSDT");
    assert(restored[0].side == hft::Side::Buy);
    assert(restored[0].price == 100);
    assert(restored[1].side == hft::Side::Sell);
    assert(restored[1].quantity == 6);
}

} // namespace

int main() {
    test_order_book_top();
    test_risk_rejects_large_order();
    test_strategy_and_simulated_fill();
    test_csv_reader();
    test_latency_stats();
    test_sequence_gap_detector();
    test_market_event_journal_round_trip();
    return 0;
}
