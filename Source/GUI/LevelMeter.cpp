#include "LevelMeter.h"

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

  // dB スケールラベル
  const float fontSize = 8.0f;
  g.setFont(juce::Font(juce::FontOptions(fontSize)));
  g.setColour(juce::Colour(0xFF888888));

  constexpr std::array<float, 5> dbMarks = {0.0f, -12.0f, -24.0f, -36.0f,
                                            -48.0f};
  for (const float db : dbMarks) {
    const float norm = juce::jmap(db, minDb, maxDb, 0.0f, 1.0f);
    const float y = bounds.getHeight() * (1.0f - norm);

    // 目盛り線
    g.drawHorizontalLine(static_cast<int>(y), barBounds.getX(),
                         barBounds.getRight());

    // ラベル(バー左側)
    juce::String label =
        (db == 0.0f) ? "0" : juce::String(static_cast<int>(db));
    g.drawText(label,
               juce::Rectangle<float>(0.0f, y - fontSize * 0.5f,
                                      labelWidth - 2.0f, fontSize),
               juce::Justification::centredRight, false);
  }
}
