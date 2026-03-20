#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/EnvelopeData.h"

using Catch::Matchers::WithinAbs;

// ─────────────────────────────────────────────────────────────────
// 1. 初期状態と基本アクセサ
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: default state", "[envelope_data]") {
  EnvelopeData env;
  CHECK_THAT(env.getDefaultValue(), WithinAbs(1.0f, 1e-6f));
  CHECK_THAT(env.getValue(), WithinAbs(1.0f, 1e-6f));
  CHECK_FALSE(env.hasPoints());
  CHECK_FALSE(env.isEnvelopeControlled());
  CHECK(env.getPoints().empty());
}

TEST_CASE("EnvelopeData: setDefaultValue / getDefaultValue", "[envelope_data]") {
  EnvelopeData env;
  env.setDefaultValue(0.42f);
  CHECK_THAT(env.getDefaultValue(), WithinAbs(0.42f, 1e-6f));
  CHECK_THAT(env.getValue(), WithinAbs(0.42f, 1e-6f));
}

// ─────────────────────────────────────────────────────────────────
// 2. addPoint の時刻ソート
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: addPoint sorts by timeMs", "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(100.0f, 0.5f);
  env.addPoint(0.0f, 1.0f);
  env.addPoint(50.0f, 0.8f);

  const auto &pts = env.getPoints();
  REQUIRE(pts.size() == 3);
  CHECK(pts[0].timeMs < pts[1].timeMs);
  CHECK(pts[1].timeMs < pts[2].timeMs);
  CHECK_THAT(pts[0].timeMs, WithinAbs(0.0f, 1e-6f));
  CHECK_THAT(pts[1].timeMs, WithinAbs(50.0f, 1e-6f));
  CHECK_THAT(pts[2].timeMs, WithinAbs(100.0f, 1e-6f));
  CHECK(env.hasPoints());
  CHECK(env.isEnvelopeControlled());
}

TEST_CASE("EnvelopeData: single point is not envelope controlled",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 0.7f);
  CHECK(env.hasPoints());
  CHECK_FALSE(env.isEnvelopeControlled());
}

// ─────────────────────────────────────────────────────────────────
// 3. setPointValue の境界
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: setPointValue valid index", "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.setPointValue(0, 0.3f);
  CHECK_THAT(env.getPoints()[0].value, WithinAbs(0.3f, 1e-6f));
}

TEST_CASE("EnvelopeData: setPointValue invalid index is no-op",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.setPointValue(-1, 0.0f);
  env.setPointValue(1, 0.0f);
  CHECK_THAT(env.getPoints()[0].value, WithinAbs(1.0f, 1e-6f));
}

// ─────────────────────────────────────────────────────────────────
// 4. setSegmentCurve のクランプ
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: setSegmentCurve clamps to [-1, +1]",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.addPoint(100.0f, 0.0f);

  env.setSegmentCurve(0, 5.0f);
  CHECK_THAT(env.getPoints()[0].curve, WithinAbs(1.0f, 1e-6f));

  env.setSegmentCurve(0, -5.0f);
  CHECK_THAT(env.getPoints()[0].curve, WithinAbs(-1.0f, 1e-6f));

  env.setSegmentCurve(0, 0.3f);
  CHECK_THAT(env.getPoints()[0].curve, WithinAbs(0.3f, 1e-6f));
}

TEST_CASE("EnvelopeData: setSegmentCurve invalid index is no-op",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.setSegmentCurve(-1, 0.5f);
  env.setSegmentCurve(1, 0.5f);
  CHECK_THAT(env.getPoints()[0].curve, WithinAbs(0.0f, 1e-6f));
}

// ─────────────────────────────────────────────────────────────────
// 5. removePoint と clearPoints
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: removePoint valid index", "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.addPoint(100.0f, 0.5f);
  env.removePoint(0);
  REQUIRE(env.getPoints().size() == 1);
  CHECK_THAT(env.getPoints()[0].timeMs, WithinAbs(100.0f, 1e-6f));
}

TEST_CASE("EnvelopeData: removePoint invalid index is no-op",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.removePoint(-1);
  env.removePoint(1);
  CHECK(env.getPoints().size() == 1);
}

TEST_CASE("EnvelopeData: clearPoints", "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.addPoint(50.0f, 0.5f);
  env.clearPoints();
  CHECK_FALSE(env.hasPoints());
  CHECK(env.getPoints().empty());
}

// ─────────────────────────────────────────────────────────────────
// 6. movePoint の全分岐
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: movePoint invalid index returns index",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  CHECK(env.movePoint(-1, 50.0f, 0.5f) == -1);
  CHECK(env.movePoint(1, 50.0f, 0.5f) == 1);
}

TEST_CASE("EnvelopeData: movePoint clamps first point", "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.addPoint(100.0f, 0.5f);
  env.addPoint(200.0f, 0.0f);

  // 先頭点: minT=0, maxT=100 → -50 は 0 にクランプ
  int idx = env.movePoint(0, -50.0f, 0.8f);
  CHECK(idx == 0);
  CHECK_THAT(env.getPoints()[0].timeMs, WithinAbs(0.0f, 1e-6f));
  CHECK_THAT(env.getPoints()[0].value, WithinAbs(0.8f, 1e-6f));
}

TEST_CASE("EnvelopeData: movePoint clamps middle point between neighbors",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.addPoint(100.0f, 0.5f);
  env.addPoint(200.0f, 0.0f);

  // 中間点: minT=0, maxT=200 → 250 は 200 にクランプ
  env.movePoint(1, 250.0f, 0.6f);
  CHECK_THAT(env.getPoints()[1].timeMs, WithinAbs(200.0f, 1e-6f));

  // 中間点: -10 は 0 にクランプ
  env.movePoint(1, -10.0f, 0.6f);
  CHECK_THAT(env.getPoints()[1].timeMs, WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("EnvelopeData: movePoint clamps last point", "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 1.0f);
  env.addPoint(100.0f, 0.5f);

  // 末尾点: minT=0, maxT=inf → -50 は 0 にクランプ
  env.movePoint(1, -50.0f, 0.2f);
  CHECK_THAT(env.getPoints()[1].timeMs, WithinAbs(0.0f, 1e-6f));
  CHECK_THAT(env.getPoints()[1].value, WithinAbs(0.2f, 1e-6f));

  // 末尾点: maxT=inf → 999 は OK
  env.movePoint(1, 999.0f, 0.1f);
  CHECK_THAT(env.getPoints()[1].timeMs, WithinAbs(999.0f, 1e-6f));
}

// ─────────────────────────────────────────────────────────────────
// 7. evaluate: ポイント数 0 / 1
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: evaluate with 0 points returns defaultValue",
          "[envelope_data]") {
  EnvelopeData env;
  env.setDefaultValue(0.42f);
  CHECK_THAT(env.evaluate(0.0f), WithinAbs(0.42f, 1e-6f));
  CHECK_THAT(env.evaluate(999.0f), WithinAbs(0.42f, 1e-6f));
}

TEST_CASE("EnvelopeData: evaluate with 1 point returns that value",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(50.0f, 0.7f);
  CHECK_THAT(env.evaluate(0.0f), WithinAbs(0.7f, 1e-6f));
  CHECK_THAT(env.evaluate(50.0f), WithinAbs(0.7f, 1e-6f));
  CHECK_THAT(env.evaluate(999.0f), WithinAbs(0.7f, 1e-6f));
}

// ─────────────────────────────────────────────────────────────────
// 8. evaluate: 範囲外（先頭前 / 末尾後）
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: evaluate before first point returns first value",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(10.0f, 0.8f);
  env.addPoint(100.0f, 0.2f);
  CHECK_THAT(env.evaluate(0.0f), WithinAbs(0.8f, 1e-6f));
  CHECK_THAT(env.evaluate(5.0f), WithinAbs(0.8f, 1e-6f));
}

TEST_CASE("EnvelopeData: evaluate after last point returns last value",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(10.0f, 0.8f);
  env.addPoint(100.0f, 0.2f);
  CHECK_THAT(env.evaluate(100.0f), WithinAbs(0.2f, 1e-6f));
  CHECK_THAT(env.evaluate(999.0f), WithinAbs(0.2f, 1e-6f));
}

// ─────────────────────────────────────────────────────────────────
// 9. evaluate: セグメント探索（3点以上）
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: evaluate selects correct segment with 3 points",
          "[envelope_data]") {
  EnvelopeData env;
  // curve=0 → 線形補間
  env.addPoint(0.0f, 0.0f);
  env.addPoint(100.0f, 1.0f);
  env.addPoint(200.0f, 0.0f);

  // 第1セグメント中間: 0→1 の中間 ≈ 0.5
  CHECK_THAT(env.evaluate(50.0f), WithinAbs(0.5f, 0.01f));
  // 第2セグメント中間: 1→0 の中間 ≈ 0.5
  CHECK_THAT(env.evaluate(150.0f), WithinAbs(0.5f, 0.01f));
  // 各端点
  CHECK_THAT(env.evaluate(0.0f), WithinAbs(0.0f, 1e-6f));
  CHECK_THAT(env.evaluate(100.0f), WithinAbs(1.0f, 0.01f));
  CHECK_THAT(env.evaluate(200.0f), WithinAbs(0.0f, 1e-6f));
}

// ─────────────────────────────────────────────────────────────────
// 10. curveLerp: 線形補間（curve ≈ 0）
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: evaluate linear interpolation with curve=0",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 0.0f);
  env.addPoint(100.0f, 1.0f);
  // curve=0 なので線形
  CHECK_THAT(env.evaluate(25.0f), WithinAbs(0.25f, 0.01f));
  CHECK_THAT(env.evaluate(75.0f), WithinAbs(0.75f, 0.01f));
}

// ─────────────────────────────────────────────────────────────────
// 11. curveLerp: 正負カーブ
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: positive and negative curve deviate from linear",
          "[envelope_data]") {
  // 正カーブ: 上に凸（t=0.5 で線形 0.5 より大きい）
  EnvelopeData envPos;
  envPos.addPoint(0.0f, 0.0f);
  envPos.addPoint(100.0f, 1.0f);
  envPos.setSegmentCurve(0, 0.8f);
  const float vPos = envPos.evaluate(50.0f);
  CHECK(vPos > 0.5f);

  // 負カーブ: 下に凸（t=0.5 で線形 0.5 より小さい）
  EnvelopeData envNeg;
  envNeg.addPoint(0.0f, 0.0f);
  envNeg.addPoint(100.0f, 1.0f);
  envNeg.setSegmentCurve(0, -0.8f);
  const float vNeg = envNeg.evaluate(50.0f);
  CHECK(vNeg < 0.5f);

  // 正と負は線形値を挟んで反対側
  CHECK(vPos > 0.5f);
  CHECK(vNeg < 0.5f);
}

// ─────────────────────────────────────────────────────────────────
// 12. t1 == t0 の防御分岐（ゼロ除算回避）
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: evaluate with coincident time points",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(50.0f, 0.3f);
  env.addPoint(50.0f, 0.9f);

  // t1==t0 → t=0 → v0 を返す
  const float v = env.evaluate(50.0f);
  CHECK(std::isfinite(v));
}

// ─────────────────────────────────────────────────────────────────
// 13. 数値健全性（NaN / Inf なし）
// ─────────────────────────────────────────────────────────────────

TEST_CASE("EnvelopeData: evaluate never returns NaN or Inf",
          "[envelope_data]") {
  EnvelopeData env;
  env.addPoint(0.0f, 0.0f);
  env.addPoint(100.0f, 2.0f);
  env.setSegmentCurve(0, 1.0f);

  for (float t = -10.0f; t <= 200.0f; t += 0.5f) {
    const float v = env.evaluate(t);
    CHECK(std::isfinite(v));
  }
}
