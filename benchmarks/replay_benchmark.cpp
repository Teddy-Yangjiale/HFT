#include "hft/exchange/simulated_exchange.hpp"
#include "hft/market_data/csv_market_data.hpp"
#include "hft/metrics/latency_stats.hpp"
#include "hft/oms/order_manager.hpp"
#include "hft/order_book/book.hpp"
#include "hft/risk/risk_engine.hpp"
#include "hft/strategy/fixed_spread_market_maker.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

using Clock = std::chrono::steady_clock;

auto elapsed_ns(Clock::time_point start, Clock::time_point end) -> std::uint64_t {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
}

void print_summary(const char* name, const hft::LatencySummary& summary) {
    std::cout << name
              << "_count=" << summary.count
              << " min_ns=" << summary.min_ns
              << " mean_ns=" << summary.mean_ns
              << " p50_ns=" << summary.p50_ns
              << " p95_ns=" << summary.p95_ns
              << " p99_ns=" << summary.p99_ns
              << " max_ns=" << summary.max_ns
              << '\n';
}

} // namespace

int main(int argc, char** argv) {
    const std::filesystem::path market_data_path = argc > 1 ? argv[1] : "data/sample_market_data.csv";
    const std::string symbol = argc > 2 ? argv[2] : "BTCUSDT";
    const int iterations = argc > 3 ? std::stoi(argv[3]) : 10000;

    hft::CsvMarketDataReader reader;
    const auto events = reader.read(market_data_path);

    hft::LatencyStats event_loop_latency(static_cast<std::size_t>(iterations) * events.size());
    hft::LatencyStats strategy_latency(static_cast<std::size_t>(iterations) * events.size());
    std::size_t total_orders = 0;
    std::size_t total_fills = 0;

    const auto benchmark_start = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        hft::OrderBook book(symbol);
        hft::RiskEngine risk({.max_single_order_qty = 10, .max_symbol_position = 100, .max_price = 10'000'000'000});
        hft::OrderManager oms(risk);
        hft::SimulatedExchange exchange;
        hft::FixedSpreadMarketMaker strategy(symbol, 1, 1);

        for (const auto& event : events) {
            const auto event_start = Clock::now();

            book.apply(event);

            const auto strategy_start = Clock::now();
            const auto requests = strategy.on_market_event(event, book);
            const auto strategy_end = Clock::now();
            strategy_latency.observe(elapsed_ns(strategy_start, strategy_end));

            for (const auto& request : requests) {
                auto order = oms.submit(request);
                ++total_orders;

                if (auto fill = exchange.match(order, book, event.exchange_ts_ns)) {
                    oms.fill(*fill);
                    ++total_fills;
                }
            }

            event_loop_latency.observe(elapsed_ns(event_start, Clock::now()));
        }
    }
    const auto benchmark_end = Clock::now();

    const auto total_events = static_cast<std::uint64_t>(iterations) * events.size();
    const auto total_ns = elapsed_ns(benchmark_start, benchmark_end);
    const auto events_per_second = total_ns == 0 ? 0 : (total_events * 1'000'000'000ULL) / total_ns;

    std::cout << "iterations=" << iterations
              << " events=" << total_events
              << " orders=" << total_orders
              << " fills=" << total_fills
              << " elapsed_ns=" << total_ns
              << " events_per_second=" << events_per_second
              << '\n';

    print_summary("event_loop", event_loop_latency.summary());
    print_summary("strategy", strategy_latency.summary());

    return 0;
}
