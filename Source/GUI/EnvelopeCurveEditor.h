#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class EnvelopeData;

/// AMP Envelope カーブエディタ
///
/// sin(t) × envelope(t) をオフライン計算して描画する。
/// ポイントなし → フラット波形（defaultValue）
/// ポイントあり → Catmull-Rom スプライン補間されたエンベロープ波形
class EnvelopeCurveEditor : public juce::Component {
public:
  explicit EnvelopeCurveEditor(EnvelopeData &data);

  void paint(juce::Graphics &g) override;

  /// 表示するサイン波のサイクル数を設定
  void setDisplayCycles(float cycles);

  /// エンベロープの表示期間（ms）を設定
  void setDisplayDurationMs(float ms);
  float getDisplayDurationMs() const { return displayDurationMs; }

  /// ポイント変更時コールバック（LUT ベイク等に使用）
  void setOnChange(std::function<void()> cb);

  // ── マウス操作（Phase 2） ──
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

private:
  // ── 座標変換ヘルパー ──
  float timeMsToX(float timeMs) const;
  float valueToY(float value) const;
  float xToTimeMs(float x) const;
  float yToValue(float y) const;

  /// ピクセル空間でのヒット判定（-1: なし）
  int findPointAtPixel(float px, float py) const;

  // ── paint() 分割ヘルパー ──
  void paintWaveform(juce::Graphics &g, float w, float h, float centreY) const;
  void paintEnvelopeOverlay(juce::Graphics &g, float w) const;
  void paintTimeline(juce::Graphics &g, float w, float h, float totalH) const;

  static constexpr float pointHitRadius = 8.0f;
  static constexpr float timelineHeight = 18.0f;

  /// プロット領域の高さ（タイムライン分を差し引いた値）
  float plotHeight() const;

  EnvelopeData &envelopeData;
  float displayDurationMs = 300.0f; // 可変にするか、後で検討
  float displayCycles = 4.0f;
  int dragPointIndex{-1};
  std::function<void()> onChange;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeCurveEditor)
};
