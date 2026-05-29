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

void test_risk_tracks_open_order_exposure() {
    hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_open_order_qty = 2, .max_price = 1'000});
    hft::OrderManager oms(risk);

    const auto first = oms.submit({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 2, 100});
    assert(first.status == hft::OrderStatus::Accepted);
    assert(risk.open_buy_quantity("BTCUSDT") == 2);

    const auto second = oms.submit({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 1, 101});
    assert(second.status == hft::OrderStatus::Rejected);
    assert(risk.open_buy_quantity("BTCUSDT") == 2);

    oms.fill({first.id, "exec-1", "BTCUSDT", hft::Side::Buy, 100, 1, 102});
    assert(risk.open_buy_quantity("BTCUSDT") == 1);
    assert(risk.position("BTCUSDT") == 1);
}

void test_risk_order_rate_limit() {
    hft::RiskEngine risk({
        .max_single_order_qty = 10,
        .max_symbol_position = 100,
        .max_open_order_qty = 100,
        .max_price = 1'000,
        .max_orders_per_window = 2,
        .order_rate_window_ns = 100,
    });

    assert(risk.check({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 1, 1'000}).accepted);
    assert(risk.check({"BTCUSDT", hft::Side::Sell, hft::OrderType::Limit, 101, 1, 1'010}).accepted);
    assert(!risk.check({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 1, 1'020}).accepted);
    assert(risk.check({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 1, 1'200}).accepted);
}

void test_risk_kill_switches() {
    hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_open_order_qty = 100, .max_price = 1'000});

    risk.set_symbol_kill_switch("BTCUSDT", true);
    assert(!risk.check({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 1, 1}).accepted);
    assert(risk.check({"ETHUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 1, 2}).accepted);

    risk.set_global_kill_switch(true);
    assert(!risk.check({"ETHUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 1, 3}).accepted);
}

void test_oms_cancel_releases_exposure() {
    hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_open_order_qty = 10, .max_price = 1'000});
    hft::OrderManager oms(risk);

    const auto order = oms.submit({"BTCUSDT", hft::Side::Sell, hft::OrderType::Limit, 101, 3, 10});
    assert(order.status == hft::OrderStatus::Accepted);
    assert(risk.open_sell_quantity("BTCUSDT") == 3);

    assert(oms.cancel(order.id));
    assert(risk.open_sell_quantity("BTCUSDT") == 0);

    const auto cancelled = oms.get(order.id);
    assert(cancelled.has_value());
    assert(cancelled->status == hft::OrderStatus::Cancelled);
    assert(!oms.cancel(order.id));
}

void test_oms_replace_cancel_replaces_with_new_order() {
    hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_open_order_qty = 10, .max_price = 1'000});
    hft::OrderManager oms(risk);

    const auto original = oms.submit({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 2, 10});
    const auto replacement = oms.replace(original.id, {"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 99, 4, 11});

    assert(replacement.has_value());
    assert(replacement->id != original.id);
    assert(replacement->status == hft::OrderStatus::Accepted);
    assert(risk.open_buy_quantity("BTCUSDT") == 4);

    const auto old = oms.get(original.id);
    assert(old.has_value());
    assert(old->status == hft::OrderStatus::Cancelled);
}

void test_oms_execution_id_dedup() {
    hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_open_order_qty = 10, .max_price = 1'000});
    hft::OrderManager oms(risk);

    const auto order = oms.submit({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 5, 10});
    oms.fill({order.id, "exec-duplicate", "BTCUSDT", hft::Side::Buy, 100, 2, 11});
    oms.fill({order.id, "exec-duplicate", "BTCUSDT", hft::Side::Buy, 100, 2, 12});

    const auto updated = oms.get(order.id);
    assert(updated.has_value());
    assert(updated->filled == 2);
    assert(risk.position("BTCUSDT") == 2);
    assert(risk.open_buy_quantity("BTCUSDT") == 3);
}

void test_oms_caps_overfill_to_remaining_quantity() {
    hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_open_order_qty = 10, .max_price = 1'000});
    hft::OrderManager oms(risk);

    const auto order = oms.submit({"BTCUSDT", hft::Side::Buy, hft::OrderType::Limit, 100, 3, 10});
    oms.fill({order.id, "exec-overfill", "BTCUSDT", hft::Side::Buy, 100, 5, 11});

    const auto updated = oms.get(order.id);
    assert(updated.has_value());
    assert(updated->filled == 3);
    assert(updated->status == hft::OrderStatus::Filled);
    assert(risk.position("BTCUSDT") == 3);
    assert(risk.open_buy_quantity("BTCUSDT") == 0);
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
    test_risk_tracks_open_order_exposure();
    test_risk_order_rate_limit();
    test_risk_kill_switches();
    test_oms_cancel_releases_exposure();
    test_oms_replace_cancel_replaces_with_new_order();
    test_oms_execution_id_dedup();
    test_oms_caps_overfill_to_remaining_quantity();
    test_strategy_and_simulated_fill();
    test_csv_reader();
    test_latency_stats();
    test_sequence_gap_detector();
    test_market_event_journal_round_trip();
    return 0;
}
