#include "ChannelFader.h"
#include "UIConstants.h"

// ────────────────────────────────────────────────────
// コンストラクタ / デストラクタ
// ────────────────────────────────────────────────────
ChannelFader::ChannelFader(juce::Colour accentColour_)
    : faderLAF(accentColour_), accentColour(accentColour_) {
  fader.setSliderStyle(juce::Slider::LinearVertical);
  fader.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
  fader.setRange(minDb, maxDb, 0.1);
  fader.setValue(0.0, juce::dontSendNotification);
  fader.setDoubleClickReturnValue(true, 0.0);
  fader.setTextValueSuffix(" dB");
  fader.setLookAndFeel(&faderLAF);
  fader.onValueChange = [this] { repaint(); };
  addAndMakeVisible(fader);

  // ── 値ラベル直接入力用 TextEditor ──
  valueEditor.setInputRestrictions(0, "0123456789.-");
  valueEditor.setFont(
      juce::Font(juce::FontOptions(UIConstants::faderValueFontSize)));
  valueEditor.setJustification(juce::Justification::centred);
  valueEditor.setColour(juce::TextEditor::backgroundColourId,
                        juce::Colour(0xFF2A2A2A));
  valueEditor.setColour(juce::TextEditor::outlineColourId, accentColour_);
  valueEditor.setColour(juce::TextEditor::textColourId, accentColour_);
  valueEditor.setColour(juce::TextEditor::focusedOutlineColourId,
                        accentColour_);
  valueEditor.onReturnKey = [this] { commitValueEditor(); };
  valueEditor.onEscapeKey = [this] {
    valueEditor.setVisible(false);
    isEditing = false;
    repaint();
  };
  valueEditor.onFocusLost = [this] { commitValueEditor(); };
  valueEditor.setVisible(false);
  addChildComponent(valueEditor);

  startTimerHz(timerHz);
}

ChannelFader::~ChannelFader() {
  stopTimer();
  fader.setLookAndFeel(nullptr);
}

// ────────────────────────────────────────────────────
// resized
// 右端 faderHandleWidth px をフェーダーに、残りをメーターに割り当て
// ────────────────────────────────────────────────────
void ChannelFader::resized() {
  auto area = getLocalBounds();
  auto faderCol = area.removeFromRight(faderHandleWidth);
  fader.setBounds(faderCol.withTrimmedTop(valueLabelHeight));
  valueEditor.setBounds(getLocalBounds().withHeight(valueLabelHeight));
}

// ────────────────────────────────────────────────────
// API
// ────────────────────────────────────────────────────
void ChannelFader::setLevelProvider(std::function<float()> provider) {
  levelProvider = std::move(provider);
}

void ChannelFader::setAccentColour(juce::Colour colour) {
  accentColour = colour;
  faderLAF.setAccent(colour);
  valueEditor.setColour(juce::TextEditor::outlineColourId, colour);
  valueEditor.setColour(juce::TextEditor::textColourId, colour);
  valueEditor.setColour(juce::TextEditor::focusedOutlineColourId, colour);
  repaint();
}

// ────────────────────────────────────────────────────
// タイマー: 30fps でレベル取得 → ピークホールド更新 → repaint
// ────────────────────────────────────────────────────
void ChannelFader::timerCallback() {
  if (!levelProvider)
    return;
  displayDb = levelProvider();

  if (displayDb >= peakDb) {
    peakDb = displayDb;
    peakHoldFrames = peakHoldFrames_;
    peakFallVelocity = 0.0f;
  } else if (peakHoldFrames > 0) {
    --peakHoldFrames;
  } else {
    peakFallVelocity += 0.05f;
    peakDb -= peakFallVelocity;
    if (peakDb < minDb) {
      peakDb = minDb;
      peakFallVelocity = 0.0f;
    }
  }

  repaint();
}

// ────────────────────────────────────────────────────
// マウスクリック
//   ラベルエリア → 値入力エディター表示
//   メーターエリア → ピークリセット
// ────────────────────────────────────────────────────
void ChannelFader::mouseDown(const juce::MouseEvent &e) {
  if (e.y < valueLabelHeight) {
    showValueEditor();
    return;
  }
  peakDb = minDb;
  peakHoldFrames = 0;
  peakFallVelocity = 0.0f;
  repaint();
}

void ChannelFader::showValueEditor() {
  valueEditor.setColour(juce::TextEditor::outlineColourId, accentColour);
  valueEditor.setColour(juce::TextEditor::textColourId, accentColour);
  valueEditor.setColour(juce::TextEditor::focusedOutlineColourId, accentColour);
  valueEditor.setText(juce::String(fader.getValue(), 1), false);
  valueEditor.setVisible(true);
  valueEditor.grabKeyboardFocus();
  valueEditor.selectAll();
  isEditing = true;
  repaint();
}

void ChannelFader::commitValueEditor() {
  if (!isEditing)
    return;
  isEditing = false;
  const float val =
      juce::jlimit(minDb, maxDb, valueEditor.getText().getFloatValue());
  fader.setValue(val, juce::sendNotificationAsync);
  valueEditor.setVisible(false);
  repaint();
}

// ────────────────────────────────────────────────────
// paint: メーターバー + ピークライン + dB スケール
// フェーダー（子 Component）は右端 faderHandleWidth px を自前描画
// ────────────────────────────────────────────────────
void ChannelFader::paint(juce::Graphics &g) {
  // ── フェーダー現在値（編集中は TextEditor が上に表示） ──
  if (!isEditing) {
    const auto valText = juce::String(fader.getValue(), 1);
    g.setFont(juce::Font(juce::FontOptions(UIConstants::faderValueFontSize)));
    g.setColour(accentColour);
    g.drawText(valText,
               juce::Rectangle<float>(0.0f, 0.0f,
                                      static_cast<float>(getWidth()),
                                      static_cast<float>(valueLabelHeight)),
               juce::Justification::centred, false);
  }

  // メーターが占有する領域（ラベル上端とフェーダーハンドルを除く）
  const auto bounds = getLocalBounds()
                          .withTrimmedTop(valueLabelHeight)
                          .withTrimmedRight(faderHandleWidth)
                          .toFloat();

  constexpr float barWidth = 8.0f;
  const float labelWidth = bounds.getWidth() - barWidth;
  const auto barBounds = bounds.withTrimmedLeft(labelWidth);

  // ── 背景 ──
  g.setColour(juce::Colour(0xFF1A1A1A));
  g.fillRoundedRectangle(barBounds, 2.0f);

  // ── レベルバー ──
  if (const float norm = juce::jmap(juce::jlimit(minDb, maxDb, displayDb),
                                    minDb, maxDb, 0.0f, 1.0f);
      norm > 0.0f) {
    const float fillH = barBounds.getHeight() * norm;
    const auto fillRect =
        barBounds.withTrimmedTop(barBounds.getHeight() - fillH);

    juce::ColourGradient gradient{
        juce::Colour(0xFF22EE22), fillRect.getBottomLeft(),
        juce::Colour(0xFFFF2222), fillRect.getTopLeft(), false};
    gradient.addColour(0.70, juce::Colour(0xFFEECC00));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(fillRect, 2.0f);
  }

  // ── 枠線 ──
  g.setColour(juce::Colours::black.withAlpha(0.5f));
  g.drawRoundedRectangle(barBounds.reduced(0.5f), 2.0f, 1.0f);

  // ── ピークホールドライン + 値 ──
  float peakYForScale = -999.0f;
  if (peakDb > minDb) {
    const float peakNorm = juce::jmap(juce::jlimit(minDb, maxDb, peakDb), minDb,
                                      maxDb, 0.0f, 1.0f);
    const float peakY = bounds.getY() + bounds.getHeight() * (1.0f - peakNorm);
    peakYForScale = peakY;

    const bool clipping = (peakDb >= 0.0f);
    g.setColour(clipping ? juce::Colour(0xFFFF4444) : juce::Colour(0xFFEEEEEE));
    g.drawHorizontalLine(static_cast<int>(peakY), barBounds.getX(),
                         barBounds.getRight());

    g.setFont(juce::Font(juce::FontOptions(UIConstants::meterFontSize)));
    g.setColour(clipping ? juce::Colour(0xFFFF4444) : accentColour);
    g.drawText(juce::String(peakDb, 1),
               juce::Rectangle<float>(0.0f, peakY - UIConstants::meterFontSize,
                                      bounds.getWidth(),
                                      UIConstants::meterFontSize),
               juce::Justification::centredLeft, false);
  }

  // ── dB スケールラベル + 目盛り線 ──
  g.setFont(juce::Font(juce::FontOptions(UIConstants::meterFontSize)));
  constexpr std::array<float, 7> dbMarks = {12.0f,  0.0f,   -12.0f, -24.0f,
                                            -36.0f, -48.0f, -60.0f};
  for (const float db : dbMarks) {
    const float norm = juce::jmap(db, minDb, maxDb, 0.0f, 1.0f);
    const float y = bounds.getY() + bounds.getHeight() * (1.0f - norm);

    g.setColour(juce::Colour(0xFF888888));
    g.drawHorizontalLine(static_cast<int>(y), barBounds.getX(),
                         barBounds.getRight());

    if (std::abs(y - peakYForScale) < 12.0f)
      continue;

    const juce::String label =
        (db == 0.0f) ? "0" : juce::String(static_cast<int>(db));
    g.setColour(juce::Colour(0xFF888888));
    g.drawText(
        label,
        juce::Rectangle<float>(0.0f, y - UIConstants::meterFontSize * 0.5f,
                               labelWidth - 2.0f, UIConstants::meterFontSize),
        juce::Justification::centredRight, false);
  }
}

// ────────────────────────────────────────────────────────────
// FaderLAF::drawLinearSlider
// トラック: 透明（メーターが背景）、◁ 三角でサムを表現
// ────────────────────────────────────────────────────────────
void ChannelFader::FaderLAF::drawLinearSlider(juce::Graphics &g, int x,
                                              int trackY, int width, int height,
                                              float sliderPos, float /*minPos*/,
                                              float /*maxPos*/,
                                              juce::Slider::SliderStyle,
                                              juce::Slider &slider) {

  // 0 dB 基準ライン（薄いグレー）
  const auto minVal = slider.getMinimum();
  const auto maxVal = slider.getMaximum();
  const auto zeroNorm = static_cast<float>((0.0 - minVal) / (maxVal - minVal));
  const float zeroY = static_cast<float>(trackY) +
                      (1.0f - zeroNorm) * static_cast<float>(height);
  g.setColour(juce::Colours::white.withAlpha(0.22f));
  g.drawHorizontalLine(static_cast<int>(zeroY), static_cast<float>(x),
                       static_cast<float>(x + width));

  // ◁ 三角形（左向き、先端が左）
  constexpr float triH = 14.0f;
  g.setColour(accent_.withAlpha(0.92f));
  juce::Path tri;
  tri.addTriangle(static_cast<float>(x), sliderPos,
                  static_cast<float>(x + width), sliderPos - triH * 0.5f,
                  static_cast<float>(x + width), sliderPos + triH * 0.5f);
  g.fillPath(tri);
}
