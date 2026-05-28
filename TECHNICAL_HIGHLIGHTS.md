# Technical Highlights

This document records the current technical core of the HFT project. It is meant to track the architecture decisions and code-level implementation choices that make the system suitable for evolving toward a low-latency trading stack.

## Architecture Highlights

### Normalized Event Boundary

The system uses `MarketEvent` as the boundary between exchange-specific feed adapters and the rest of the trading stack. Strategies, risk, OMS, replay, and simulation consume normalized events instead of raw venue messages.

Why this matters:

- Exchange adapters can be swapped without rewriting strategies.
- Replay can drive the same strategy and OMS code path as live trading.
- Feed-specific complexity is isolated from the trading core.

Key files:

- `include/hft/core/market_event.hpp`
- `include/hft/market_data/csv_market_data.hpp`

### Sequence Gap Detection Before Book Mutation

`SequenceGapDetector` checks event sequence continuity before an event mutates the local order book.

Why this matters:

- A stale or corrupted book is worse than no book.
- Gap detection is a required first step before snapshot recovery.
- The current behavior skips gap events; the next production step is symbol halt plus snapshot recovery.

Key files:

- `include/hft/market_data/sequence_gap_detector.hpp`
- `src/sequence_gap_detector.cpp`

### Replayable Trading Loop

The replay app runs the same logical chain that live trading will use:

```text
market event -> gap check -> order book -> strategy -> risk/OMS -> simulated exchange -> fills
```

Why this matters:

- Backtests, paper trading, and live trading can converge toward one code path.
- Bugs become easier to reproduce.
- Latency can be measured at stable boundaries.

Key files:

- `src/main.cpp`
- `benchmarks/replay_benchmark.cpp`

### Binary Normalized Event Journal

`MarketEventJournal` writes and reads normalized market events in a compact binary format with a magic header and version.

Why this matters:

- Deterministic replay is possible without reparsing CSV.
- Journal versioning gives us room to evolve the format.
- It is a foundation for audit trails and incident reproduction.

Current limitation:

- The journal stores normalized events, not raw exchange packets. Production systems should store both.

Key files:

- `include/hft/replay/market_event_journal.hpp`
- `src/market_event_journal.cpp`

### Explicit OMS and Risk Boundary

The OMS owns local order lifecycle state. The risk engine is the mandatory pre-trade gate before orders can proceed.

Why this matters:

- Strategy output is treated as intent, not permission to trade.
- Risk logic can be expanded without changing every strategy.
- Gateway-specific exchange IDs can later be mapped back to local OMS IDs.

Key files:

- `include/hft/oms/order_manager.hpp`
- `src/order_manager.cpp`
- `include/hft/risk/risk_engine.hpp`
- `src/risk_engine.cpp`

### Measurement-Led Optimization

The project includes a benchmark app and latency summary utility. Optimizations should be judged against repeatable measurements, not intuition.

Why this matters:

- Low latency work without measurement usually optimizes the wrong path.
- p95 and p99 latency matter more than average latency for trading stability.
- Release builds and CPU pinning are part of the measurement process.

Key files:

- `include/hft/metrics/latency_stats.hpp`
- `benchmarks/replay_benchmark.cpp`
- `scripts/linux_perf_env_report.sh`

## Code Implementation Highlights

### Allocation-Aware Strategy Output

Strategies append `OrderRequest` objects into caller-owned storage:

```cpp
virtual void on_market_event(
    const MarketEvent& event,
    const OrderBook& book,
    std::vector<OrderRequest>& output
) = 0;
```

Why this matters:

- The event loop can reserve once and reuse the same output buffer.
- Strategy evaluation avoids constructing a fresh vector on every market event.
- This pattern is a stepping stone toward fixed-capacity stack buffers.

Key files:

- `include/hft/strategy/strategy.hpp`
- `src/fixed_spread_market_maker.cpp`

### OMS Capacity Reservation

`OrderManager::reserve_orders()` allows benchmark and replay callers to pre-size the order map when the expected order volume is known.

Why this matters:

- Avoids repeated hash table rehashing during benchmark hot loops.
- Makes latency distribution less noisy.
- Keeps the optimization explicit at the call site.

Key files:

- `include/hft/oms/order_manager.hpp`
- `src/order_manager.cpp`

### Narrow Order Book API

The order book exposes `apply()` and `top()` instead of exposing internal containers.

Why this matters:

- The current `std::map` implementation can later be replaced with a cache-aware ladder.
- Strategies do not depend on the internal book representation.
- The API makes the exact update contract visible: absolute quantity updates, zero removes a level.

Key files:

- `include/hft/order_book/book.hpp`
- `src/book.cpp`

### Hot Path Comments Describe Contracts

Comments focus on correctness assumptions and latency tradeoffs rather than restating syntax.

Examples:

- Whether an event update is absolute or incremental.
- Whether a method may allocate, block, or perform I/O.
- Why a simulation result should not be treated as production PnL.

## Current Performance Baseline

The benchmark command is:

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
./build-release/hft_benchmark_app data/sample_market_data.csv BTCUSDT 50000
```

Recent local WSL2 baseline before the allocation-aware strategy change:

```text
events_per_second=5544488
event_loop p50=114ns
event_loop p95=276ns
strategy p50=30ns
```

Recent local WSL2 baseline after the allocation-aware strategy change and OMS reservation:

```text
events_per_second=6258648
event_loop p50=109ns
event_loop p95=149ns
strategy p50=34ns
```

This is a useful directionally positive result, not a final latency claim. WSL2 scheduling noise and the tiny sample feed make exact numbers unstable, so future benchmark reports should include multiple pinned Release runs.

Benchmark numbers from WSL2 are development signals only. Production latency validation should eventually happen on native Linux with pinned cores and a real NIC.

## Next Low-Latency Targets

1. Replace strategy output `std::vector` with a fixed-capacity small buffer.
2. Replace `std::map` order book internals with a cache-aware price ladder.
3. Add open-order exposure and rate-limit checks without allocations.
4. Add fixed-size event and order pools.
5. Add structured latency measurement for each stage: book, strategy, risk, OMS, exchange gateway.
6. Add CPU affinity helpers for benchmark and live process startup.
7. Move from normalized-only journaling to raw packet capture plus normalized journaling.
