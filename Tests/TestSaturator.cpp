#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/Saturator.h"

using namespace Catch::Matchers;

// ─── driveGainFromDb ────────────────────────────────────────

TEST_CASE("Saturator: driveGainFromDb(0) returns unity gain", "[saturator]")
{
    REQUIRE_THAT(Saturator::driveGainFromDb(0.0f), WithinAbs(1.0f, 1e-6f));
}

TEST_CASE("Saturator: driveGainFromDb(20) returns 10x gain", "[saturator]")
{
    REQUIRE_THAT(Saturator::driveGainFromDb(20.0f), WithinAbs(10.0f, 1e-4f));
}

// ─── ゼロ入力 ───────────────────────────────────────────────

TEST_CASE("Saturator: Soft mode zero input produces zero output", "[saturator]")
{
    REQUIRE_THAT(Saturator::process(0.0f, 0.0f, static_cast<int>(Saturator::ClipType::Soft)),
                 WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("Saturator: Hard mode zero input produces zero output", "[saturator]")
{
    REQUIRE_THAT(Saturator::process(0.0f, 0.0f, static_cast<int>(Saturator::ClipType::Hard)),
                 WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("Saturator: Tube mode zero input produces expected bias (DC offset by design)", "[saturator]")
{
    // Tube: (0 + 0.1) / (1 + |0 + 0.1|) = 0.1 / 1.1 ≈ 0.09090...
    // DirectEngine の早期リターンで本バイアスのリークを防止している（回帰テスト）
    constexpr float bias     = 0.1f;
    constexpr float expected = bias / (1.0f + bias); // ≈ 0.0909f
    REQUIRE_THAT(Saturator::process(0.0f, 0.0f, static_cast<int>(Saturator::ClipType::Tube)),
                 WithinAbs(expected, 1e-6f));
}

// ─── Hard クリップ境界 ──────────────────────────────────────

TEST_CASE("Saturator: Hard clip is bounded to [-1, 1] for large inputs", "[saturator]")
{
    REQUIRE(Saturator::process( 100.0f, 24.0f, static_cast<int>(Saturator::ClipType::Hard)) <= 1.0f);
    REQUIRE(Saturator::process(-100.0f, 24.0f, static_cast<int>(Saturator::ClipType::Hard)) >= -1.0f);
}

// ─── 単調性 ─────────────────────────────────────────────────

TEST_CASE("Saturator: each ClipType is monotonically non-decreasing", "[saturator]")
{
    for (int type = 0; type <= 2; ++type)
    {
        const float y1 = Saturator::process(-0.5f, 0.0f, type);
        const float y2 = Saturator::process( 0.0f, 0.0f, type);
        const float y3 = Saturator::process( 0.5f, 0.0f, type);
        REQUIRE(y1 <= y2);
        REQUIRE(y2 <= y3);
    }
}
