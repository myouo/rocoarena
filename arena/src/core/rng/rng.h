#pragma once

#include <cstdint>
#include <random>
#include <type_traits>

// 统一的随机工具：支持设置种子、概率判定、范围随机。
// 提供线程局部实例，避免跨线程锁竞争。
class RNG {
public:
    using Engine = std::mt19937_64;

    // 获取线程局部实例；默认使用随机种子。
    static RNG& instance() {
        thread_local RNG rng{defaultSeed()};
        return rng;
    }

    explicit RNG(std::uint64_t seed = defaultSeed()) : engine_(seed), seed_(seed) {}

    void reseed(std::uint64_t seed) {
        seed_ = seed;
        engine_.seed(seed_);
    }

    std::uint64_t seed() const { return seed_; }

    // [0, 1) 实数。
    double uniform() {
        return std::generate_canonical<double, 64>(engine_);
    }

    // 给定概率命中；概率范围外自动裁剪到 [0,1]。
    bool chance(double probability) {
        if (probability <= 0.0) return false;
        if (probability >= 1.0) return true;
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(engine_) < probability;
    }

    // 整数闭区间 [min, max]。
    template <typename Int>
    Int range(Int min, Int max) {
        static_assert(std::is_integral_v<Int>, "range(min,max) requires integral type");
        std::uniform_int_distribution<Int> dist(min, max);
        return dist(engine_);
    }

    // 实数闭区间 [min, max]。
    template <typename Real>
    Real rangeReal(Real min, Real max) {
        static_assert(std::is_floating_point_v<Real>, "rangeReal(min,max) requires floating type");
        std::uniform_real_distribution<Real> dist(min, max);
        return dist(engine_);
    }

private:
    static std::uint64_t defaultSeed() {
        std::random_device rd;
        std::uint64_t high = static_cast<std::uint64_t>(rd()) << 32;
        std::uint64_t low = static_cast<std::uint64_t>(rd());
        return high ^ low;
    }

    Engine engine_;
    std::uint64_t seed_;
};
