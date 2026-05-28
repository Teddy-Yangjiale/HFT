#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

namespace hft {

struct LatencySummary {
    std::uint64_t count{0};
    std::uint64_t min_ns{0};
    std::uint64_t max_ns{0};
    std::uint64_t mean_ns{0};
    std::uint64_t p50_ns{0};
    std::uint64_t p95_ns{0};
    std::uint64_t p99_ns{0};
};

class LatencyStats {
public:
    explicit LatencyStats(std::size_t reserve_count = 0) {
        samples_ns_.reserve(reserve_count);
    }

    void observe(std::uint64_t latency_ns) {
        samples_ns_.push_back(latency_ns);
    }

    [[nodiscard]] auto summary() const -> LatencySummary {
        if (samples_ns_.empty()) {
            return {};
        }

        auto sorted = samples_ns_;
        std::ranges::sort(sorted);

        const auto total = std::accumulate(sorted.begin(), sorted.end(), std::uint64_t{0});
        return {
            .count = static_cast<std::uint64_t>(sorted.size()),
            .min_ns = sorted.front(),
            .max_ns = sorted.back(),
            .mean_ns = total / sorted.size(),
            .p50_ns = percentile(sorted, 50),
            .p95_ns = percentile(sorted, 95),
            .p99_ns = percentile(sorted, 99),
        };
    }

    [[nodiscard]] auto size() const -> std::size_t {
        return samples_ns_.size();
    }

private:
    // This helper uses nearest-rank percentile semantics. It is simple and stable
    // for benchmark reporting; production telemetry can later swap this for an
    // HDR histogram or t-digest without changing callers.
    static auto percentile(const std::vector<std::uint64_t>& sorted, std::uint64_t pct) -> std::uint64_t {
        const auto index = ((sorted.size() - 1) * pct) / 100;
        return sorted[index];
    }

    // We intentionally keep raw samples in the MVP because replay benchmarks are
    // bounded and deterministic. For live trading, this should become a fixed-size
    // histogram to avoid unbounded memory growth on long sessions.
    std::vector<std::uint64_t> samples_ns_;
};

} // namespace hft
