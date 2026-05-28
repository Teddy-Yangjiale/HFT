# HFT

C++20 high-frequency trading system MVP. This repository starts with a small, dependency-light event-driven trading loop that can be compiled and tested locally before adding real exchange connectivity.

## Current MVP

- CSV market data replay
- Level 2 order book top-of-book maintenance
- Strategy interface
- Fixed-spread market making example strategy
- Pre-trade risk checks
- Order manager with basic lifecycle state
- Simulated exchange fill model
- Unit-style smoke tests through CTest

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

```bash
./build/hft_replay_app data/sample_market_data.csv BTCUSDT
```

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
