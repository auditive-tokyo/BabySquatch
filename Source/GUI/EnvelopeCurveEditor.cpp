#include "EnvelopeCurveEditor.h"
#include "../DSP/EnvelopeData.h"
#include "UIConstants.h"

EnvelopeCurveEditor::EnvelopeCurveEditor(EnvelopeData &data)
    : envelopeData(data) {
  setOpaque(true);
}

void EnvelopeCurveEditor::paint(juce::Graphics &g) {
  const auto bounds = getLocalBounds().toFloat();
  const float w = bounds.getWidth();
  const float h = bounds.getHeight();
  const float centreY = h * 0.5f;

  // ── Layer 1: 背景 ──
  g.fillAll(UIConstants::Colours::waveformBg);

  if (w < 1.0f || h < 1.0f)
    return;

  // ── Layer 2: フラット波形塗りつぶし（sin(t) × defaultValue）──
  const float amplitude = envelopeData.getValue();
  // amplitude 1.0 → ピークがコンポーネント上下端に到達
  // amplitude 2.0 → クリップ（パスは領域内に留まる）
  const float scaledAmp = std::min(amplitude, 2.0f) * centreY;

  const auto numPoints = static_cast<int>(w);

  // 波形パス（塗りつぶし用）
  juce::Path fillPath;
  fillPath.startNewSubPath(0.0f, centreY);

  for (int i = 0; i <= numPoints; ++i) {
    const auto x = static_cast<float>(i);
    const float t = x / w;
    const float sinVal =
        std::sin(t * displayCycles * juce::MathConstants<float>::twoPi);
    const float y = juce::jlimit(0.0f, h, centreY - sinVal * scaledAmp);
    fillPath.lineTo(x, y);
  }

  fillPath.lineTo(w, centreY);
  fillPath.closeSubPath();

  // oomphArc グラデーション塗りつぶし（半透明）
  g.setColour(UIConstants::Colours::oomphArc.withAlpha(0.3f));
  g.fillPath(fillPath);

  // ── 波形アウトライン ──
  juce::Path waveLine;
  for (int i = 0; i <= numPoints; ++i) {
    const auto x = static_cast<float>(i);
    const float t = x / w;
    const float sinVal =
        std::sin(t * displayCycles * juce::MathConstants<float>::twoPi);
    const float y = juce::jlimit(0.0f, h, centreY - sinVal * scaledAmp);

    if (i == 0)
      waveLine.startNewSubPath(x, y);
    else
      waveLine.lineTo(x, y);
  }

  g.setColour(UIConstants::Colours::oomphArc);
  g.strokePath(waveLine, juce::PathStrokeType(1.5f));

  // ── センターライン（ゼロクロッシング基準） ──
  g.setColour(juce::Colours::white.withAlpha(0.08f));
  g.drawHorizontalLine(static_cast<int>(centreY), 0.0f, w);
}

void EnvelopeCurveEditor::setDisplayCycles(float cycles) {
  displayCycles = cycles;
  repaint();
}

void EnvelopeCurveEditor::setDisplayDurationMs(float ms) {
  displayDurationMs = ms;
  repaint();
}

void EnvelopeCurveEditor::setOnChange(std::function<void()> cb) {
  onChange = std::move(cb);
}

// ── 座標変換ヘルパー ──

float EnvelopeCurveEditor::timeMsToX(float timeMs) const {
  const auto w = static_cast<float>(getWidth());
  return (displayDurationMs > 0.0f) ? (timeMs / displayDurationMs) * w : 0.0f;
}

float EnvelopeCurveEditor::valueToY(float value) const {
  // value 0.0 → 下端、value 2.0 → 上端、value 1.0 → 中央
  const auto h = static_cast<float>(getHeight());
  return h - (value / 2.0f) * h;
}

float EnvelopeCurveEditor::xToTimeMs(float x) const {
  const auto w = static_cast<float>(getWidth());
  return (w > 0.0f) ? (x / w) * displayDurationMs : 0.0f;
}

float EnvelopeCurveEditor::yToValue(float y) const {
  const auto h = static_cast<float>(getHeight());
  return (h > 0.0f) ? (1.0f - y / h) * 2.0f : 1.0f;
}

int EnvelopeCurveEditor::findPointAtPixel(float px, float py) const {
  const auto &pts = envelopeData.getPoints();
  const float r2 = pointHitRadius * pointHitRadius;

  for (int i = 0; i < static_cast<int>(pts.size()); ++i) {
    const float dx = px - timeMsToX(pts[static_cast<size_t>(i)].timeMs);
    const float dy = py - valueToY(pts[static_cast<size_t>(i)].value);
    if (dx * dx + dy * dy <= r2)
      return i;
  }
  return -1;
}

// ── マウス操作（Step 3/4 で本実装） ──

void EnvelopeCurveEditor::mouseDoubleClick(const juce::MouseEvent & /*e*/) {
  // Step 4 で実装
}

void EnvelopeCurveEditor::mouseDown(const juce::MouseEvent & /*e*/) {
  // Step 4 で実装
}

void EnvelopeCurveEditor::mouseDrag(const juce::MouseEvent & /*e*/) {
  // Step 4 で実装
}

void EnvelopeCurveEditor::mouseUp(const juce::MouseEvent & /*e*/) {
  // Step 4 で実装
}
