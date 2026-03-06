#include "LevelMeter.h"

#include "UIConstants.h"
#include <array>

LevelMeter::LevelMeter() { startTimerHz(timerHz); }

LevelMeter::~LevelMeter() { stopTimer(); }

void LevelMeter::setLevelProvider(std::function<float()> provider) {
  levelProvider = std::move(provider);
}

void LevelMeter::timerCallback() {
  if (!levelProvider)
    return;
  displayDb = levelProvider();

  // ── ピークホールド更新 ──
  if (displayDb >= peakDb) {
    peakDb = displayDb;
    peakHoldFrames = peakHoldFrames_;
    peakFallVelocity = 0.0f;
  } else if (peakHoldFrames > 0) {
    --peakHoldFrames;
  } else {
    // 保持終了 → フォールバック
    peakFallVelocity += 0.05f; // 加速
    peakDb -= peakFallVelocity;
    if (peakDb < minDb) {
      peakDb = minDb;
      peakFallVelocity = 0.0f;
    }
  }

  repaint();
}

void LevelMeter::mouseDown(const juce::MouseEvent &) {
  // クリックでピークリセット
  peakDb = minDb;
  peakHoldFrames = 0;
  peakFallVelocity = 0.0f;
  repaint();
}

void LevelMeter::paint(juce::Graphics &g) {
  const auto bounds = getLocalBounds().toFloat();
  const float barWidth = 8.0f;
  const float labelWidth = bounds.getWidth() - barWidth;

  // バー領域（右側）
  const auto barBounds = bounds.withTrimmedLeft(labelWidth);

  // 背景
  g.setColour(juce::Colour(0xFF1A1A1A));
  g.fillRoundedRectangle(barBounds, 2.0f);

  // レベルバー
  if (const float normalised = juce::jmap(juce::jlimit(minDb, maxDb, displayDb),
                                          minDb, maxDb, 0.0f, 1.0f);
      normalised > 0.0f) {
    const float fillH = barBounds.getHeight() * normalised;
    const auto fillRect =
        barBounds.withTrimmedTop(barBounds.getHeight() - fillH);

    juce::ColourGradient gradient{
        juce::Colour(0xFF22EE22), fillRect.getBottomLeft(),
        juce::Colour(0xFFFF2222), fillRect.getTopLeft(), false};
    gradient.addColour(0.70, juce::Colour(0xFFEECC00));

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(fillRect, 2.0f);
  }

  // 枠線
  g.setColour(juce::Colours::black.withAlpha(0.5f));
  g.drawRoundedRectangle(barBounds.reduced(0.5f), 2.0f, 1.0f);

  // ── ピークホールドライン＋値 ──
  if (peakDb > minDb) {
    const float peakNorm = juce::jmap(juce::jlimit(minDb, maxDb, peakDb), minDb,
                                      maxDb, 0.0f, 1.0f);
    const float peakY = bounds.getHeight() * (1.0f - peakNorm);

    // ライン色: クリッピング(>= 0 dB)は赤, 通常は白
    const bool clipping = (peakDb >= 0.0f);
    g.setColour(clipping ? juce::Colour(0xFFFF4444) : juce::Colour(0xFFEEEEEE));
    g.drawHorizontalLine(static_cast<int>(peakY), barBounds.getX(),
                         barBounds.getRight());

    // 値テキスト（バー左側のラベルエリアに表示）
    const auto peakText = juce::String(peakDb, 1);
    g.setFont(juce::Font(juce::FontOptions(UIConstants::meterFontSize)));
    g.setColour(clipping ? juce::Colour(0xFFFF4444) : accentColour);
    g.drawText(peakText,
               juce::Rectangle<float>(0.0f, peakY - UIConstants::meterFontSize,
                                      bounds.getWidth(),
                                      UIConstants::meterFontSize),
               juce::Justification::centredLeft, false);
  }

  // dB スケールラベル
  g.setFont(juce::Font(juce::FontOptions(UIConstants::meterFontSize)));

  // ピーク値と近傍のラベルを非表示にするための Y 座標
  const float peakNormForScale =
      juce::jmap(juce::jlimit(minDb, maxDb, peakDb), minDb, maxDb, 0.0f, 1.0f);
  const float peakYForScale =
      (peakDb > minDb) ? bounds.getHeight() * (1.0f - peakNormForScale)
                       : -999.0f;

  constexpr std::array<float, 5> dbMarks = {0.0f, -12.0f, -24.0f, -36.0f,
                                            -48.0f};
  for (const float db : dbMarks) {
    const float norm = juce::jmap(db, minDb, maxDb, 0.0f, 1.0f);
    const float y = bounds.getHeight() * (1.0f - norm);

    // 目盛り線
    g.setColour(juce::Colour(0xFF888888));
    g.drawHorizontalLine(static_cast<int>(y), barBounds.getX(),
                         barBounds.getRight());

    // ピーク値と近い（12px以内）ラベルはスキップ
    if (std::abs(y - peakYForScale) < 12.0f)
      continue;

    juce::String label =
        (db == 0.0f) ? "0" : juce::String(static_cast<int>(db));
    g.drawText(label,
               juce::Rectangle<float>(0.0f,
                                      y - UIConstants::meterFontSize * 0.5f,
                                      labelWidth - 2.0f,
                                      UIConstants::meterFontSize),
               juce::Justification::centredRight, false);
  }
}
