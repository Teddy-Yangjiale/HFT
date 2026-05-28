# HFT

C++20 high-frequency trading system MVP. This repository starts with a small, dependency-light event-driven trading loop that can be compiled and tested locally before adding real exchange connectivity.

## Project Status

This is currently an engineering MVP, not a production trading stack. The code is useful for building the core loop, replay discipline, and latency measurement habits before connecting real capital.

## Current MVP

- CSV market data replay
- Level 2 order book top-of-book maintenance
- Strategy interface
- Fixed-spread market making example strategy
- Pre-trade risk checks
- Order manager with basic lifecycle state
- Simulated exchange fill model
- Latency summary utility for local benchmarks
- Replay benchmark executable
- Unit-style smoke tests through CTest

## Current Gaps

The biggest gaps versus a real high-frequency trading system are:

1. **Market data correctness**
   - No live adapter yet.
   - No packet gap detection beyond the sequence field being present.
   - No snapshot recovery.
   - The order book applies absolute CSV quantities only.

2. **Order management**
   - No cancel/replace workflow.
   - No exchange order ID mapping.
   - No execution ID deduplication.
   - No gateway session state, sequence numbers, heartbeats, or reconnect logic.

3. **Risk**
   - Current checks cover single-order size, position, and price sanity.
   - Missing open-order exposure, order-rate limits, cancel-rate limits, max loss, symbol kill switch, and global kill switch.

4. **Simulation quality**
   - The simulated exchange only fills crossing orders at top of book.
   - It does not model queue position, latency, fees, maker/taker behavior, partial queue depletion, or adverse selection.

5. **Performance engineering**
   - `std::map`, `std::vector`, and virtual strategy dispatch are acceptable for the MVP but not final hot-path choices.
   - No CPU pinning, memory pool, fixed-capacity queues, or NUMA policy is wired into the runtime yet.
   - Latency stats are in-memory raw samples; production telemetry should use bounded histograms.

6. **Operations**
   - No persistent binary journal.
   - No structured audit log.
   - No metrics exporter.
   - No process supervisor, config hot reload, or emergency control plane.

## Build

Use a native WSL/Linux shell for the most stable build and benchmark behavior:

```bash
cmake -S . -B build-wsl -G Ninja
cmake --build build-wsl
ctest --test-dir build-wsl --output-on-failure
```

## Run

```bash
./build-wsl/hft_replay_app data/sample_market_data.csv BTCUSDT
```

Expected sample output:

```text
events=6 orders=10 fills=0 position=0 top_bid=100002 top_ask=100008
```

## Benchmark

Run a deterministic replay benchmark:

```bash
./build-wsl/hft_benchmark_app data/sample_market_data.csv BTCUSDT 100000
```

Pin the benchmark to one CPU core to reduce scheduler noise:

```bash
taskset -c 2 ./build-wsl/hft_benchmark_app data/sample_market_data.csv BTCUSDT 100000
```

The benchmark reports total events per second plus event-loop and strategy latency summaries:

```text
iterations=100000 events=600000 orders=1000000 fills=0 elapsed_ns=...
event_loop_count=... min_ns=... mean_ns=... p50_ns=... p95_ns=... p99_ns=... max_ns=...
strategy_count=... min_ns=... mean_ns=... p50_ns=... p95_ns=... p99_ns=... max_ns=...
```

## Local Environment Potential

Run the environment report:

```bash
bash scripts/linux_perf_env_report.sh
```

For local low-latency experiments, prioritize these steps:

1. Build and run inside WSL/Linux, not through Windows UNC build paths.
2. Use `Release` builds for performance numbers:

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
taskset -c 2 ./build-release/hft_benchmark_app data/sample_market_data.csv BTCUSDT 100000
```

3. Keep the benchmark process pinned to one physical core.
4. Close noisy background workloads while measuring.
5. Check CPU governor. `performance` is preferable to `powersave` for stable latency.
6. Install `numactl`, `ethtool`, and `linux-tools` when you start network and CPU profiling.
7. Treat WSL benchmark numbers as development signals. Production latency validation should eventually happen on native Linux with a real NIC.

Useful tools later:

- `perf stat`
- `perf record`
- `taskset`
- `numactl`
- `ethtool`
- `tcpdump`
- eBPF/bcc tools

## Code Commenting Standard

Comments should explain system contracts, latency tradeoffs, and correctness assumptions. Avoid comments that repeat obvious C++ syntax. Important examples:

- Explain whether an update is absolute or incremental.
- Explain whether a timestamp comes from the exchange, kernel, NIC, or application.
- Explain whether a method is allowed to allocate, block, log, or perform I/O.
- Explain simulation limitations so backtest results are not mistaken for live expectations.

## Near-Term Build Plan

The next engineering milestones should be:

1. Add a binary event journal for deterministic replay and audit.
2. Extend OMS with cancel/replace and execution ID deduplication.
3. Add open-order exposure, rate limits, and kill switches to risk.
4. Replace the current fill model with a queue-position-aware simulator.
5. Replace `std::map` order book internals with a cache-friendlier price ladder after benchmarks justify it.
6. Add structured metrics output for replay and paper trading.
7. Add one real market data adapter while keeping the normalized event boundary unchanged.

## Roadmap

1. Harden event model with explicit timestamps for receive, strategy decision, send, ack, and fill.
2. Add deterministic binary event journal for replay and audit.
3. Replace the simple fill model with queue-position-aware simulation.
4. Add live market data adapters behind the same normalized event interface.
5. Add exchange gateways for paper trading first, then production sessions.
6. Add kill switch, loss limits, order-rate limits, cancel-rate limits, and symbol-level controls.
7. Add latency histograms and operational metrics.
8. Profile hot paths and then introduce lock-free queues, CPU pinning, NUMA controls, and kernel-bypass networking only where measurements justify it.

## Design Notes

The first version optimizes for correctness and observability over raw latency. The system should be able to replay the same market event stream and reproduce strategy, risk, and OMS behavior before real capital is connected.
