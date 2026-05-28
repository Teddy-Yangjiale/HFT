#!/usr/bin/env bash
set -euo pipefail

echo "== System =="
uname -a
echo

echo "== CPU =="
lscpu || true
echo

echo "== CPU Frequency Governors =="
if compgen -G "/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor" > /dev/null; then
  sort -V /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor | uniq -c
else
  echo "cpufreq governor files not available"
fi
echo

echo "== NUMA =="
if command -v numactl >/dev/null 2>&1; then
  numactl --hardware
else
  echo "numactl not installed"
fi
echo

echo "== Huge Pages =="
grep -E "HugePages|Hugepagesize" /proc/meminfo || true
echo

echo "== Kernel Network Queues =="
for iface in /sys/class/net/*; do
  name="$(basename "$iface")"
  [ "$name" = "lo" ] && continue
  echo "-- $name --"
  if command -v ethtool >/dev/null 2>&1; then
    ethtool -l "$name" 2>/dev/null || echo "queue info unavailable"
    ethtool -k "$name" 2>/dev/null | grep -E "generic-receive-offload|generic-segmentation-offload|tcp-segmentation-offload|rx-checksumming|tx-checksumming" || true
  else
    echo "ethtool not installed"
  fi
done
echo

echo "== Scheduler / Limits =="
ulimit -a
echo

echo "== Suggested Next Checks =="
echo "1. Prefer native Linux or WSL2 Ubuntu terminal builds with Ninja for stable timing."
echo "2. Pin benchmarks with: taskset -c <core> ./build-wsl/hft_benchmark_app data/sample_market_data.csv BTCUSDT 100000"
echo "3. For production-like tests, disable CPU powersave, isolate hot cores, and avoid running from Windows UNC paths."
