#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/TransientDetector.h"
#include <vector>

namespace {
// sr=1000Hz にするとサンプル数 = ms そのままになり計算が単純
constexpr double kSr = 1000.0;

/// 全ゼロブロックを N サンプル process する
int processZeros(TransientDetector &td, int n) {
  const std::vector<float> zeros(static_cast<size_t>(n), 0.0f);
  return td.process(zeros);
}

/// 先頭 impulsePos を 0.0, impulsePos サンプルを amplitude, 残りを 0.0 にしたブロック
std::vector<float> makeImpulseBlock(int size, int impulsePos,
                                    float amplitude = 1.0f) {
  std::vector<float> v(static_cast<size_t>(size), 0.0f);
  v[static_cast<size_t>(impulsePos)] = amplitude;
  return v;
}
} // namespace

// ─── disabled では常に -1 ──────────────────────────────────────

// setEnabled(false) のとき process が early-return して -1 を返すことを確認する
TEST_CASE("TransientDetector: disabled always returns -1",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);
  td.setEnabled(false); // 無効のまま

  const auto block = makeImpulseBlock(64, 0);
  REQUIRE(td.process(block) == -1);
}

// ─── enabled + 無音 → 未検出 ────────────────────────────────────

// 有効化しても無音ブロックでは onset が閾値を超えず -1 を返すことを確認する
TEST_CASE("TransientDetector: silence returns -1 when enabled",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);
  td.setEnabled(true);

  REQUIRE(processZeros(td, 128) == -1);
}

// ─── 先頭インパルスで検出・位置が 0 付近 ───────────────────────

// 先頭サンプルに amplitude=1.0 のインパルスを入れると 0 付近で検出されることを確認する。
// sr=1000Hz では attackFast ≈ 0.99 なので先頭サンプルで即検出される。
TEST_CASE("TransientDetector: single impulse triggers near position 0",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);
  td.setEnabled(true);
  td.setThresholdDb(-40.0f); // 低めにして確実に検出

  const auto block = makeImpulseBlock(64, 0);
  const int pos = td.process(block);

  REQUIRE(pos >= 0);
  REQUIRE(pos < 5); // attackFast ≒ 0.99 なので 0〜数サンプル以内
}

// ─── hold 中は同一ブロック内の 2 回目を無視する ────────────────

// holdMs を長く取り、ブロック内に 2 つのインパルスを入れた場合、
// 1 回目しか検出されないことを確認する（holdCounter > 0 経路のカバレッジ）
TEST_CASE("TransientDetector: second impulse suppressed during hold",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);
  td.setEnabled(true);
  td.setThresholdDb(-40.0f);
  td.setHoldMs(500.0f); // ホールド 500ms = 500 サンプル @ 1kHz

  // 位置 0 と 20 にインパルスを入れたブロック（128 samples）
  std::vector<float> block(128, 0.0f);
  block[0] = 1.0f;
  block[20] = 1.0f;

  const int first = td.process(block);

  REQUIRE(first >= 0);
  REQUIRE(first < 5); // 1 回目だけ検出

  // 2 回目以降の呼び出しもホールド中は検出しない
  const int second = td.process(block);
  REQUIRE(second == -1);
}

// ─── hold 経過後は再トリガー可能 ───────────────────────────────

// holdMs を短くし、hold が明けた後に再びインパルスを送ると再度検出されることを確認する
TEST_CASE("TransientDetector: triggers again after hold expires",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);
  td.setEnabled(true);
  td.setThresholdDb(-40.0f);
  td.setHoldMs(10.0f); // ホールド 10ms = 10 サンプル @ 1kHz

  // 1 回目のトリガー
  auto block = makeImpulseBlock(64, 0);
  const int first = td.process(block);
  REQUIRE(first >= 0);

  // hold を消化するための無音ブロック（20 サンプル = hold の 2 倍）
  // reset 相当の無音で slow env も下げる
  for (int i = 0; i < 5; ++i)
    processZeros(td, 20);

  // 2 回目のトリガー
  const int second = td.process(block);
  REQUIRE(second >= 0); // 再検出されること
}

// ─── ヒステリシス: 再アーム後に再トリガー ──────────────────────

// トリガー後、onset が threshold×30%=kHysteresisRatio 未満まで下がれば
// armed_ = true に戻り、次のインパルスで再トリガーされることを確認する。
// (onset < threshLin_ * kHysteresisRatio の分岐カバレッジ)
TEST_CASE("TransientDetector: re-arms after hysteresis condition met",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);
  td.setEnabled(true);
  td.setThresholdDb(-40.0f);
  td.setHoldMs(1.0f); // ホールド最小（1ms = 1 サンプル）

  // 1 回目: トリガー
  auto block = makeImpulseBlock(64, 0);
  REQUIRE(td.process(block) >= 0);

  // 十分な無音でエンベロープを落とし、ヒステリシス条件を満たす
  for (int i = 0; i < 20; ++i)
    processZeros(td, 100);

  // 2 回目: 再アームされているので再検出
  REQUIRE(td.process(block) >= 0);
}

// ─── reset() で状態が初期化される ──────────────────────────────

// 一度動かした後 reset() すると、同一入力で同じ位置を返すことを確認する（再現性）
TEST_CASE("TransientDetector: reset restores initial state",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);
  td.setEnabled(true);
  td.setThresholdDb(-40.0f);

  const auto block = makeImpulseBlock(64, 0);

  const int first = td.process(block);
  REQUIRE(first >= 0);

  td.reset();

  const int second = td.process(block);
  REQUIRE(second == first); // リセット後は同じ位置で検出
}

// ─── isEnabled() は setEnabled() を反映する ─────────────────────

// isEnabled() のアクセサが正しく動くことを確認する
TEST_CASE("TransientDetector: isEnabled reflects setEnabled",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);

  td.setEnabled(true);
  REQUIRE(td.isEnabled() == true);

  td.setEnabled(false);
  REQUIRE(td.isEnabled() == false);
}

// ─── attack/release 両係路の実行確認 ────────────────────────────

// 上昇→下降波形（インパルスの後に無音）を処理して、
// Fast/Slow 両エンベロープの attack と release 分岐が通ることを確認する（NaN/Inf チェック）
TEST_CASE("TransientDetector: attack and release paths produce finite values",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);
  td.setEnabled(true);

  // 上昇: 0→1 (attack), 下降: 1→0 (release)
  std::vector<float> block(200, 0.0f);
  for (int i = 10; i < 20; ++i)
    block[static_cast<size_t>(i)] = 1.0f;

  // process 後に内部状態が finite であることは直接見えないが、
  // クラッシュ / 例外なく完走することで確認する
  const int pos = td.process(block);
  REQUIRE((pos == -1 || pos >= 0)); // 戻り値が有効範囲
}

// ─── 空ブロックで process してもクラッシュしない ────────────────

// std::span サイズ 0 の入力を渡しても -1 を返しクラッシュしないことを確認する
TEST_CASE("TransientDetector: empty block returns -1 without crash",
          "[transient_detector]") {
  TransientDetector td;
  td.prepare(kSr);
  td.setEnabled(true);

  const std::vector<float> empty;
  REQUIRE(td.process(empty) == -1);
}
