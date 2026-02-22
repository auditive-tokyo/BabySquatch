#include "OomphOscillator.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <juce_core/juce_core.h>

// ── 帯域境界（Hz）: 20, 40, 80, … , 10240, 20480 ──
static constexpr std::array<float, OomphOscillator::numBands + 1> bandEdges = {
    20.0f,   40.0f,   80.0f,   160.0f,   320.0f,  640.0f,
    1280.0f, 2560.0f, 5120.0f, 10240.0f, 20480.0f};

// ────────────────────────────────────────────────────
// prepareToPlay
// ────────────────────────────────────────────────────
void OomphOscillator::prepareToPlay(double newSampleRate) {
  sampleRate = newSampleRate;
  currentIndex = 0.0f;
  buildAllTables();
  // デフォルトポインタを Sine band-0 に設定
  activeSineTable = tables[0][0].data();
  activeShapeTable = activeSineTable;
}

// ── 各波形の1サンプル計算ヘルパー（file-scope static） ──

static double computeTriSample(double phase, int maxHarmonic) {
  double sum = 0.0;
  for (int n = 1; n <= maxHarmonic; n += 2) {
    const double sign = ((n / 2) % 2 == 0) ? 1.0 : -1.0;
    sum += sign * std::sin(static_cast<double>(n) * phase) /
           static_cast<double>(n * n);
  }
  return sum * (8.0 / (juce::MathConstants<double>::pi *
                       juce::MathConstants<double>::pi));
}

static double computeSquareSample(double phase, int maxHarmonic) {
  double sum = 0.0;
  for (int n = 1; n <= maxHarmonic; n += 2)
    sum += std::sin(static_cast<double>(n) * phase) / static_cast<double>(n);
  return sum * (4.0 / juce::MathConstants<double>::pi);
}

static double computeSawSample(double phase, int maxHarmonic) {
  double sum = 0.0;
  for (int n = 1; n <= maxHarmonic; ++n) {
    const double sign = (n % 2 == 1) ? 1.0 : -1.0;
    sum += sign * std::sin(static_cast<double>(n) * phase) /
           static_cast<double>(n);
  }
  return sum * (2.0 / juce::MathConstants<double>::pi);
}

/// 1波形 × 1帯域のテーブルを埋める
static void buildShapeBandTable(std::vector<float> &tbl, WaveShape ws,
                                int maxHarmonic) {
  constexpr auto twoPi = juce::MathConstants<double>::twoPi;
  for (int i = 0; i < OomphOscillator::tableSize; ++i) {
    const double phase = twoPi * i / OomphOscillator::tableSize;
    double sample = 0.0;
    switch (ws) {
    using enum WaveShape;
    case Sine:   sample = std::sin(phase); break;
    case Tri:    sample = computeTriSample(phase, maxHarmonic); break;
    case Square: sample = computeSquareSample(phase, maxHarmonic); break;
    case Saw:    sample = computeSawSample(phase, maxHarmonic); break;
    }
    tbl[static_cast<size_t>(i)] = static_cast<float>(sample);
  }
  tbl[static_cast<size_t>(OomphOscillator::tableSize)] = tbl[0]; // wrap用
}

// ────────────────────────────────────────────────────
// buildAllTables — 4波形 × 10帯域を事前計算
// ────────────────────────────────────────────────────
void OomphOscillator::buildAllTables() {
  const auto nyquist = static_cast<float>(sampleRate * 0.5);

  for (int shape = 0; shape < numShapes; ++shape) {
    const auto ws = static_cast<WaveShape>(shape);
    for (int band = 0; band < numBands; ++band) {
      auto &tbl = tables[static_cast<size_t>(shape)][static_cast<size_t>(band)];
      tbl.assign(static_cast<size_t>(tableSize + 1), 0.0f);

      const float bandTop = bandEdges[static_cast<size_t>(band + 1)];
      const int maxHarmonic =
          std::max(1, static_cast<int>(std::floor(nyquist / bandTop)));

      buildShapeBandTable(tbl, ws, maxHarmonic);
    }
  }
}

// ────────────────────────────────────────────────────
// bandIndexForFreq — 周波数 → 帯域インデックス (0〜9)
// ────────────────────────────────────────────────────
int OomphOscillator::bandIndexForFreq(float hz) {
  // 帯域は対数スケール（倍々）: 20,40,80,...,10240,20480
  // log2(hz/20) でオクターブ番号を得て clamp
  if (hz <= bandEdges[0])
    return 0;
  const auto band = static_cast<int>(std::floor(std::log2(hz / bandEdges[0])));
  return std::clamp(band, 0, numBands - 1);
}

// ────────────────────────────────────────────────────
// triggerNote / stopNote
// ────────────────────────────────────────────────────
void OomphOscillator::triggerNote() {
  active = true;
  currentIndex = 0.0f;
}

void OomphOscillator::stopNote() {
  active = false;
  tableDelta = 0.0f;
  currentIndex = 0.0f;
}

// ────────────────────────────────────────────────────
// setFrequencyHz — tableDelta 更新 + 帯域選択
// ────────────────────────────────────────────────────
void OomphOscillator::setFrequencyHz(float hz) {
  tableDelta = static_cast<float>(hz * tableSize / sampleRate);

  const int band = bandIndexForFreq(hz);
  activeBand = band;

  const auto bandIdx = static_cast<size_t>(band);
  // Sine テーブルは常に更新（BLEND で参照するため）
  activeSineTable = tables[0][bandIdx].data();
  // 選択波形テーブルも更新
  const auto shapeIdx = static_cast<size_t>(currentShape.load());
  activeShapeTable = tables[shapeIdx][bandIdx].data();
}

// ────────────────────────────────────────────────────
// setWaveShape / getWaveShape
// ────────────────────────────────────────────────────
void OomphOscillator::setWaveShape(WaveShape shape) {
  currentShape.store(static_cast<int>(shape));
}

WaveShape OomphOscillator::getWaveShape() const {
  return static_cast<WaveShape>(currentShape.load());
}

// ────────────────────────────────────────────────────
// readTable — テーブルから線形補間で1サンプル
// ────────────────────────────────────────────────────
float OomphOscillator::readTable(const float *table) const {
  const auto index0 = static_cast<size_t>(currentIndex);
  const auto index1 = index0 + 1;
  const float frac = currentIndex - static_cast<float>(index0);
  return table[index0] + frac * (table[index1] - table[index0]);
}

// ────────────────────────────────────────────────────
// getNextSample — 現在は activeSineTable のみ読み出し
//   （Phase 3 で BLEND クロスフェード追加予定）
// ────────────────────────────────────────────────────
float OomphOscillator::getNextSample() {
  if (!active)
    return 0.0f;

  // Phase 1: サイン波テーブルのみ（既存動作と同一）
  const float sample = readTable(activeSineTable);

  currentIndex += tableDelta;
  if (currentIndex >= static_cast<float>(tableSize))
    currentIndex -= static_cast<float>(tableSize);

  return sample;
}
