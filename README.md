# HFT

C++20 high-frequency trading system MVP. This repository starts with a small, dependency-light event-driven trading loop that can be compiled and tested locally before adding real exchange connectivity.

## Project Status

This is currently an engineering MVP, not a production trading stack. The code is useful for building the core loop, replay discipline, and latency measurement habits before connecting real capital.

For a concise record of the project's architecture and implementation highlights, see [TECHNICAL_HIGHLIGHTS.md](TECHNICAL_HIGHLIGHTS.md).

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
- Sequence gap detector before order book mutation
- Binary normalized market event journal for deterministic replay
- Allocation-aware strategy output buffer
- OMS order-map capacity reservation for benchmarks/replay
- Risk hardening stage 1: open-order exposure, order-rate limit, global/symbol kill switches
- Unit-style smoke tests through CTest

## Current Gaps

The biggest gaps versus a real high-frequency trading system are:

1. **Market data correctness**
   - No live adapter yet.
   - Basic sequence gap detection exists, but recovery is still manual.
   - No snapshot recovery.
   - The order book applies absolute CSV quantities only.
   - No raw packet capture, so feed adapter bugs cannot yet be replayed from original exchange bytes.

2. **Order management**
   - No cancel/replace workflow.
   - No exchange order ID mapping.
   - No execution ID deduplication.
   - No gateway session state, sequence numbers, heartbeats, or reconnect logic.

3. **Risk**
   - Current checks cover single-order size, price sanity, position, open-order exposure, order-rate limits, and kill switches.
   - Missing cancel-rate limits, max loss, account-level risk, strategy-level risk, and persistent kill-switch control.

4. **Simulation quality**
   - The simulated exchange only fills crossing orders at top of book.
   - It does not model queue position, latency, fees, maker/taker behavior, partial queue depletion, or adverse selection.

5. **Performance engineering**
   - `std::map` order book internals and virtual strategy dispatch are acceptable for the MVP but not final hot-path choices.
   - Strategy output now reuses caller-owned storage, but a fixed-capacity buffer would be tighter than `std::vector`.
   - No CPU pinning, memory pool, fixed-capacity queues, or NUMA policy is wired into the runtime yet.
   - Latency stats are in-memory raw samples; production telemetry should use bounded histograms.

6. **Operations**
   - Binary normalized event journal exists, but raw packet capture is not implemented.
   - No structured audit log.
   - No metrics exporter.
   - No process supervisor, config hot reload, or emergency control plane.

## Industrial-Grade Difference

An industrial HFT stack is not just faster code. It is a controlled system where every state transition can be explained under packet loss, venue disconnects, duplicate fills, clock drift, risk-limit changes, and process restarts. The main differences are:

- **Feed handling:** production systems validate sequence numbers, detect gaps before book mutation, recover from snapshots, and preserve raw packets for postmortems.
- **Time discipline:** production systems record exchange, NIC, kernel, application receive, decision, send, ack, and fill timestamps with known clock synchronization quality.
- **OMS correctness:** production systems treat every order transition as a state-machine event, deduplicate exchange messages, reconcile after reconnect, and preserve audit trails.
- **Risk as hard infrastructure:** production systems enforce position, open-order exposure, message-rate limits, cancel-rate limits, loss limits, fat-finger checks, and kill switches before order send.
- **Simulation realism:** production-grade research uses queue position, latency, maker/taker fees, partial fills, cancels, rejects, and adverse selection instead of top-of-book crossing only.
- **Operational control:** production deployments include metrics, alerting, configuration control, runbooks, process supervision, credential isolation, and disaster recovery.
- **Performance validation:** production latency work is measurement-led, using release builds, CPU pinning, profiling, cache-aware structures, bounded allocations, and native Linux hardware tests.

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
events=6 orders=10 fills=0 gaps=0 position=0 top_bid=100002 top_ask=100008
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

## Upgrade Plan

The project will be upgraded in this order. Each stage should preserve replay determinism, tests, and benchmark coverage.

### Stage 1: Risk Hardening

Status: in progress.

Implemented:

- Open-order exposure tracking by symbol and side.
- Order-rate limit using a timestamp window.
- Global kill switch.
- Symbol-level kill switch.
- OMS integration so accepted orders reserve exposure and fills/rejects release exposure.

Still needed:

- Cancel-rate limit.
- Max loss limit.
- Strategy-level kill switch.
- Persistent operator control for kill-switch state.
- Metrics for accepted/rejected orders by reason.

### Stage 2: OMS State Machine

Planned:

- Explicit cancel workflow.
- Replace/amend workflow.
- Exchange order ID mapping.
- Execution ID deduplication.
- Reconnect reconciliation hooks.
- State-transition tests for every legal and illegal transition.

### Stage 3: Order Book Optimization

Planned:

- Replace `std::map` with a cache-friendly price ladder or flat book.
- Keep the public `OrderBook::apply()` and `OrderBook::top()` API stable.
- Benchmark old and new book implementations under Release builds.
- Add gap/snapshot recovery hooks before mutation.

### Stage 4: Queue-Position Simulator

Planned:

- Track queue position for resting orders.
- Model partial fills from trade prints and level depletion.
- Add maker/taker fee model.
- Add configurable latency and cancel-failure behavior.
- Report simulated fill confidence separately from realized live fills.

### Stage 5: Market Data Upgrade

Planned:

- Raw packet journal next to normalized event journal.
- Snapshot recovery after sequence gaps.
- Explicit trading halt behavior when a book is unsafe.
- Live adapter boundary for exchange/vendor feeds.
- Feed latency timestamps: exchange, receive, normalize, book apply.

### Stage 6: Paper Trading Adapter

Planned:

- Real exchange sandbox or paper gateway.
- Gateway session heartbeat and reconnect.
- Order ack/reject/fill mapping into OMS.
- Credentials isolated from source code.
- Paper-trading metrics and audit logs.

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
