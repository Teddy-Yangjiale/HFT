#include "hft/exchange/simulated_exchange.hpp"
#include "hft/market_data/csv_market_data.hpp"
#include "hft/metrics/latency_stats.hpp"
#include "hft/oms/order_manager.hpp"
#include "hft/order_book/book.hpp"
#include "hft/risk/risk_engine.hpp"
#include "hft/strategy/fixed_spread_market_maker.hpp"

#include <cassert>
#include <filesystem>

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
    const auto requests = strategy.on_market_event({hft::MarketEventType::BookUpdate, "BTCUSDT", hft::Side::Sell, 104, 5, 2, 2}, book);
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

} // namespace

int main() {
    test_order_book_top();
    test_risk_rejects_large_order();
    test_strategy_and_simulated_fill();
    test_csv_reader();
    test_latency_stats();
    return 0;
}
