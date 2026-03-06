#include "MasterFader.h"
#include "UIConstants.h"

// ────────────────────────────────────────────────────
// コンストラクタ
// ────────────────────────────────────────────────────
MasterFader::MasterFader() {
  // ── ラベル ──
  label.setText("MASTER", juce::dontSendNotification);
  label.setFont(juce::Font(juce::FontOptions(UIConstants::fontSizeSmall)));
  label.setColour(juce::Label::textColourId, UIConstants::Colours::labelText);
  label.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(label);

  // ── フェーダー ──
  fader.setSliderStyle(juce::Slider::LinearHorizontal);
  fader.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
  fader.setRange(minDb, maxDb, 0.1);
  fader.setValue(0.0, juce::dontSendNotification);
  fader.setDoubleClickReturnValue(true, 0.0);
  fader.setTextValueSuffix(" dB");

  // メーターバーを透かすためトラック・背景を透明に、サムは白
  fader.setColour(juce::Slider::backgroundColourId,
                  juce::Colours::transparentBlack);
  fader.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
  fader.setColour(juce::Slider::thumbColourId, UIConstants::Colours::text);

  fader.onValueChange = [this] {
    if (onValueChange)
      onValueChange(static_cast<float>(fader.getValue()));
  };
  addAndMakeVisible(fader);

  startTimerHz(timerHz);
}

MasterFader::~MasterFader() { stopTimer(); }

// ────────────────────────────────────────────────────
// API
// ────────────────────────────────────────────────────
void MasterFader::setOnValueChange(std::function<void(float)> cb) {
  onValueChange = std::move(cb);
}

void MasterFader::setLevelProvider(int ch, std::function<float()> provider) {
  levelProvider[static_cast<std::size_t>(ch & 1)] = std::move(provider);
}

// ────────────────────────────────────────────────────
// タイマー: 30fps でレベル取得→ピークホールド更新→ repaint
// ────────────────────────────────────────────────────
void MasterFader::timerCallback() {
  bool anyActive = false;
  for (std::size_t ch = 0; ch < 2; ++ch) {
    if (!levelProvider[ch])
      continue;
    anyActive = true;
    auto &m = meter[ch];
    m.displayDb = levelProvider[ch]();
    if (m.displayDb >= m.peakDb) {
      m.peakDb = m.displayDb;
      m.peakHoldFrames = peakHoldFrames_;
      m.peakFallVelocity = 0.0f;
    } else if (m.peakHoldFrames > 0) {
      --m.peakHoldFrames;
    } else {
      m.peakFallVelocity += 0.05f;
      m.peakDb -= m.peakFallVelocity;
      if (m.peakDb < minDb) {
        m.peakDb = minDb;
        m.peakFallVelocity = 0.0f;
      }
    }
  }
  if (anyActive)
    repaint();
}

// ────────────────────────────────────────────────────
// paint: L/R 横向きレベルメーターバー2段（フェーダーの背後に描画）
// ────────────────────────────────────────────────────
void MasterFader::paint(juce::Graphics &g) {
  if (const bool hasProvider = levelProvider[0] || levelProvider[1]; !hasProvider)
    return;

  auto area = getLocalBounds();
  area.removeFromTop(labelHeight);
  const auto meterArea = area.reduced(4, 4).toFloat();

  // L/R を縦に2分割
  const float barH = (meterArea.getHeight() - 2.0f) * 0.5f;
  const std::array<juce::Rectangle<float>, 2> bars = {
      {meterArea.withHeight(barH),
       meterArea.withY(meterArea.getY() + barH + 2.0f).withHeight(barH)}};
  const std::array<juce::String, 2> chLabels = {{"L", "R"}};

  const float zeroNorm = juce::jmap(0.0f, minDb, maxDb, 0.0f, 1.0f);

  for (std::size_t ch = 0; ch < 2; ++ch) {
    const auto &bar = bars[ch];
    const auto &m = meter[ch];

    // 背景
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.fillRoundedRectangle(bar, 2.0f);

    // レベルバー
    if (const float norm = juce::jmap(juce::jlimit(minDb, maxDb, m.displayDb),
                                      minDb, maxDb, 0.0f, 1.0f);
        norm > 0.0f) {
      const auto fill = bar.withWidth(bar.getWidth() * norm);
      juce::ColourGradient grad{juce::Colour(0xFF22EE22), fill.getTopLeft(),
                                juce::Colour(0xFFFF2222), fill.getTopRight(),
                                false};
      grad.addColour(0.70, juce::Colour(0xFFEECC00));
      g.setGradientFill(grad);
      g.fillRoundedRectangle(fill, 2.0f);
    }

    // 枠線
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(bar.reduced(0.5f), 2.0f, 1.0f);

    // ピークライン
    if (m.peakDb > minDb) {
      const float peakNorm = juce::jmap(juce::jlimit(minDb, maxDb, m.peakDb),
                                        minDb, maxDb, 0.0f, 1.0f);
      const float peakX = bar.getX() + bar.getWidth() * peakNorm;
      g.setColour(m.peakDb >= 0.0f ? juce::Colour(0xFFFF4444)
                                   : juce::Colour(0xFFEEEEEE));
      g.drawVerticalLine(static_cast<int>(peakX), bar.getY(), bar.getBottom());
    }

    // 0 dB 基準ライン
    {
      const float zeroX = bar.getX() + bar.getWidth() * zeroNorm;
      g.setColour(juce::Colours::white.withAlpha(0.25f));
      g.drawVerticalLine(static_cast<int>(zeroX), bar.getY(), bar.getBottom());
    }

    // L / R ラベル
    g.setFont(juce::Font(juce::FontOptions(UIConstants::meterFontSize)));
    g.setColour(juce::Colour(0xFF888888));
    g.drawText(chLabels[ch], bar.withWidth(12.0f),
               juce::Justification::centred, false);
  }
}

// ────────────────────────────────────────────────────
// resized
// ────────────────────────────────────────────────────
void MasterFader::resized() {
  auto area = getLocalBounds();
  label.setBounds(area.removeFromTop(labelHeight));
  fader.setBounds(area); // paint() と同じ領域: meter が後ろ、fader サムが前
}
