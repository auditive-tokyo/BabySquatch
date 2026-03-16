#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/EnvelopeLutManager.h"

#include <array>
#include <vector>

using namespace Catch::Matchers;

// ── reset ───────────────────────────────────────────

// reset() 後は両バッファが 1.0 で埋まり、activeIndex が 0 になること
TEST_CASE("EnvelopeLutManager: reset fills lut with 1.0", "[envelope_lut]") {
  EnvelopeLutManager mgr;
  // bake で何か書き込んでおく
  std::vector<float> ramp(64);
  for (int i = 0; i < 64; ++i)
    ramp[static_cast<std::size_t>(i)] = static_cast<float>(i) / 63.0f;
  mgr.bake(ramp.data(), 64);

  mgr.reset();
  const auto &lut = mgr.getActiveLut();
  for (int i = 0; i < EnvelopeLutManager::lutSize; ++i)
    CHECK(lut[static_cast<std::size_t>(i)] == 1.0f);
}

// ── bake (リサンプル & ダブルバッファフリップ) ──────

// 2点 {0.0, 1.0} を bake すると LUT が 0→1 のランプになること
TEST_CASE("EnvelopeLutManager: bake resamples linear ramp", "[envelope_lut]") {
  EnvelopeLutManager mgr;
  mgr.reset();

  const std::array<float, 2> src = {0.0f, 1.0f};
  mgr.bake(src.data(), 2);

  const auto &lut = mgr.getActiveLut();
  CHECK(lut[0] == 0.0f);
  CHECK(lut[EnvelopeLutManager::lutSize - 1] == 1.0f);
  // 中間点は≈0.5
  const auto mid = static_cast<std::size_t>(EnvelopeLutManager::lutSize / 2);
  CHECK_THAT(lut[mid], WithinAbs(0.5, 0.01));
}

// bake を2回呼ぶとダブルバッファが交互にフリップすること
TEST_CASE("EnvelopeLutManager: double buffer flips on each bake",
          "[envelope_lut]") {
  EnvelopeLutManager mgr;
  mgr.reset();

  // 1回目: 全部 0.25
  const std::array<float, 1> d1 = {0.25f};
  mgr.bake(d1.data(), 1);
  CHECK(mgr.getActiveLut()[0] == 0.25f);

  // 2回目: 全部 0.75 → もう一方のバッファに書かれてフリップ
  const std::array<float, 1> d2 = {0.75f};
  mgr.bake(d2.data(), 1);
  CHECK(mgr.getActiveLut()[0] == 0.75f);
}

// ── setDurationMs / getDurationMs ───────────────────

// durationMs のデフォルト値を確認し、set/get が正しく動くこと
TEST_CASE("EnvelopeLutManager: duration ms get/set", "[envelope_lut]") {
  EnvelopeLutManager mgr;
  CHECK(mgr.getDurationMs() == 300.0f);

  mgr.setDurationMs(500.0f);
  CHECK(mgr.getDurationMs() == 500.0f);
}

// ── computeAmp: 基本 ───────────────────────────────

// 均一 LUT (1.0) で duration 内ならamp≈1.0 を返すこと
TEST_CASE("EnvelopeLutManager: computeAmp returns 1.0 for flat lut",
          "[envelope_lut]") {
  std::array<float, EnvelopeLutManager::lutSize> lut{};
  lut.fill(1.0f);

  // duration 冒頭
  CHECK_THAT(EnvelopeLutManager::computeAmp(lut, 300.0f, 0.0f),
             WithinAbs(1.0, 1e-5));
  // duration 中盤（fadeOut 開始前）
  CHECK_THAT(EnvelopeLutManager::computeAmp(lut, 300.0f, 100.0f),
             WithinAbs(1.0, 1e-5));
}

// ── computeAmp: LUT 補間 ───────────────────────────

// ランプ LUT (0→1) の中間時刻で≈0.5 を返すこと
TEST_CASE("EnvelopeLutManager: computeAmp reads ramp lut at midpoint",
          "[envelope_lut]") {
  std::array<float, EnvelopeLutManager::lutSize> lut{};
  for (int i = 0; i < EnvelopeLutManager::lutSize; ++i)
    lut[static_cast<std::size_t>(i)] =
        static_cast<float>(i) /
        static_cast<float>(EnvelopeLutManager::lutSize - 1);

  // t=150ms / dur=300ms → LUT 中央 ≈ 0.5（ただし fadeOut 開始前）
  const float amp = EnvelopeLutManager::computeAmp(lut, 300.0f, 150.0f);
  CHECK_THAT(amp, WithinAbs(0.5, 0.01));
}

// ── computeAmp: 末尾 5ms half-cosine フェードアウト ─

// duration 末尾で 0 に近づくこと（半余弦フェード）
TEST_CASE("EnvelopeLutManager: computeAmp fades to zero at end",
          "[envelope_lut]") {
  std::array<float, EnvelopeLutManager::lutSize> lut{};
  lut.fill(1.0f);

  const float durMs = 300.0f;
  // ちょうど duration 位置 → t=1.0 → cos(π)=-1 → 0.5*(1-1)=0
  const float ampAtEnd = EnvelopeLutManager::computeAmp(lut, durMs, durMs);
  CHECK_THAT(ampAtEnd, WithinAbs(0.0, 1e-4));

  // fadeOut 中間 (297.5ms) → t=0.5 → cos(π/2)=0 → 0.5*(1+0)=0.5
  const float ampAtMid =
      EnvelopeLutManager::computeAmp(lut, durMs, durMs - 2.5f);
  CHECK_THAT(ampAtMid, WithinAbs(0.5, 0.01));
}

// ── computeAmp: duration=0 の境界ケース ─────────────

// ampDurMs=0 のとき lutIdx=0 を参照し、即座にフェードアウトすること
TEST_CASE("EnvelopeLutManager: computeAmp with zero duration",
          "[envelope_lut]") {
  std::array<float, EnvelopeLutManager::lutSize> lut{};
  lut.fill(0.8f);

  // t=0, dur=0 → lutPos=0, fadeStart=max(0, -5)=0 → t=min(0/5,1)=0 →
  // cos(0)=1 → amp = 0.8 * 0.5*(1+1) = 0.8
  CHECK_THAT(EnvelopeLutManager::computeAmp(lut, 0.0f, 0.0f),
             WithinAbs(0.8, 1e-5));

  // t=5ms, dur=0 → fadeStart=0, t=min(5/5,1)=1 → cos(π)=-1 → amp=0
  CHECK_THAT(EnvelopeLutManager::computeAmp(lut, 0.0f, 5.0f),
             WithinAbs(0.0, 1e-4));
}

// ── computeAmp: duration を大きく超過した場合 ───────

// noteTimeMs が duration+5ms を超えたら確実に 0 になること
TEST_CASE("EnvelopeLutManager: computeAmp returns 0 well past duration",
          "[envelope_lut]") {
  std::array<float, EnvelopeLutManager::lutSize> lut{};
  lut.fill(1.0f);

  const float amp = EnvelopeLutManager::computeAmp(lut, 300.0f, 1000.0f);
  CHECK_THAT(amp, WithinAbs(0.0, 1e-4));
}
