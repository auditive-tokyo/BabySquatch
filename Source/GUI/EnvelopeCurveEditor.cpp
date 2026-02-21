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

  const auto numPixels = static_cast<int>(w);
  const bool hasEnvPoints = envelopeData.hasPoints();

  // ── Layer 2 & 3: 波形（塗りつぶし + アウトライン）──
  // ポイントあり → sin(t) × evaluate(timeMs)
  // ポイントなし → sin(t) × defaultValue（フラット）
  juce::Path fillPath;
  juce::Path waveLine;
  fillPath.startNewSubPath(0.0f, centreY);

  for (int i = 0; i <= numPixels; ++i) {
    const auto x = static_cast<float>(i);
    const float t = x / w;
    const float sinVal =
        std::sin(t * displayCycles * juce::MathConstants<float>::twoPi);

    const float amplitude = hasEnvPoints
                                ? envelopeData.evaluate(t * displayDurationMs)
                                : envelopeData.getValue();
    const float scaledAmp = std::min(amplitude, 2.0f) * centreY;
    const float y = juce::jlimit(0.0f, h, centreY - sinVal * scaledAmp);

    fillPath.lineTo(x, y);

    if (i == 0)
      waveLine.startNewSubPath(x, y);
    else
      waveLine.lineTo(x, y);
  }

  fillPath.lineTo(w, centreY);
  fillPath.closeSubPath();

  // oomphArc グラデーション塗りつぶし（半透明）
  g.setColour(UIConstants::Colours::oomphArc.withAlpha(0.3f));
  g.fillPath(fillPath);

  g.setColour(UIConstants::Colours::oomphArc);
  g.strokePath(waveLine, juce::PathStrokeType(1.5f));

  // ── センターライン（ゼロクロッシング基準） ──
  g.setColour(juce::Colours::white.withAlpha(0.08f));
  g.drawHorizontalLine(static_cast<int>(centreY), 0.0f, w);

  // ── Layer 4 & 5: エンベロープカーブ + コントロールポイント ──
  if (hasEnvPoints) {
    // エンベロープカーブライン
    juce::Path envLine;
    for (int i = 0; i <= numPixels; ++i) {
      const auto x = static_cast<float>(i);
      const float timeMs = (x / w) * displayDurationMs;
      const float ey = valueToY(envelopeData.evaluate(timeMs));

      if (i == 0)
        envLine.startNewSubPath(x, ey);
      else
        envLine.lineTo(x, ey);
    }

    g.setColour(UIConstants::Colours::oomphArc.brighter(0.4f));
    g.strokePath(envLine, juce::PathStrokeType(1.5f));

    // コントロールポイント
    const auto &pts = envelopeData.getPoints();
    for (int i = 0; i < static_cast<int>(pts.size()); ++i) {
      const auto idx = static_cast<size_t>(i);
      const float px = timeMsToX(pts[idx].timeMs);
      const float py = valueToY(pts[idx].value);
      constexpr float r = pointHitRadius * 0.6f;

      if (i == dragPointIndex) {
        // ドラッグ中: フル白ハイライト
        g.setColour(juce::Colours::white);
        g.fillEllipse(px - r, py - r, r * 2.0f, r * 2.0f);
      } else {
        // 通常: 半透明塗り + 白枠
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.fillEllipse(px - r, py - r, r * 2.0f, r * 2.0f);
        g.setColour(juce::Colours::white);
        g.drawEllipse(px - r, py - r, r * 2.0f, r * 2.0f, 1.0f);
      }
    }
  }
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

// ── マウス操作 ──

void EnvelopeCurveEditor::mouseDoubleClick(const juce::MouseEvent &e) {
  const auto px = static_cast<float>(e.x);
  const auto py = static_cast<float>(e.y);
  if (const int hit = findPointAtPixel(px, py); hit >= 0) {
    envelopeData.removePoint(hit);
  } else {
    const float timeMs = std::clamp(xToTimeMs(px), 0.0f, displayDurationMs);
    const float value = std::clamp(yToValue(py), 0.0f, 2.0f);
    envelopeData.addPoint(timeMs, value);
  }

  if (onChange)
    onChange();
  repaint();
}

void EnvelopeCurveEditor::mouseDown(const juce::MouseEvent &e) {
  dragPointIndex =
      findPointAtPixel(static_cast<float>(e.x), static_cast<float>(e.y));
}

void EnvelopeCurveEditor::mouseDrag(const juce::MouseEvent &e) {
  if (dragPointIndex < 0)
    return;

  const float timeMs =
      std::clamp(xToTimeMs(static_cast<float>(e.x)), 0.0f, displayDurationMs);
  const float value = std::clamp(yToValue(static_cast<float>(e.y)), 0.0f, 2.0f);
  dragPointIndex = envelopeData.movePoint(dragPointIndex, timeMs, value);

  if (onChange)
    onChange();
  repaint();
}

void EnvelopeCurveEditor::mouseUp(const juce::MouseEvent & /*e*/) {
  dragPointIndex = -1;
}
