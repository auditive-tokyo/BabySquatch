// ClickParams.cpp
// Click panel UI setup / layout
#include "../PluginEditor.h"

namespace {
void styleFilterBox(juce::TextEditor &box, const juce::String &defaultVal,
                    const juce::Font &font) {
  box.setFont(font);
  box.setText(defaultVal, juce::dontSendNotification);
  box.setJustification(juce::Justification::centred);
  box.setInputRestrictions(10, "0123456789.");
  box.setColour(juce::TextEditor::backgroundColourId,
                UIConstants::Colours::waveformBg);
  box.setColour(juce::TextEditor::textColourId,
                juce::Colours::white.withAlpha(0.90f));
  box.setColour(juce::TextEditor::outlineColourId,
                UIConstants::Colours::clickArc.withAlpha(0.30f));
  box.setColour(juce::TextEditor::focusedOutlineColourId,
                UIConstants::Colours::clickArc.withAlpha(0.80f));
}

void styleFilterLabel(juce::Label &label, const juce::String &text,
                      const juce::Font &font) {
  label.setText(text, juce::dontSendNotification);
  label.setFont(font);
  label.setColour(juce::Label::textColourId, UIConstants::Colours::labelText);
  label.setJustificationType(juce::Justification::centredRight);
}
} // namespace

void BabySquatchAudioProcessorEditor::setupClickParams() {
  // ── Mode label ──
  const auto smallFont =
      juce::Font(juce::FontOptions(UIConstants::fontSizeMedium));
  clickUI.modeLabel.setText("mode:", juce::dontSendNotification);
  clickUI.modeLabel.setFont(smallFont);
  clickUI.modeLabel.setColour(juce::Label::textColourId,
                              UIConstants::Colours::labelText);
  clickUI.modeLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(clickUI.modeLabel);

  // ── Mode combo ──
  clickUI.modeCombo.addItem("Tone",   static_cast<int>(ClickUI::Mode::Tone));
  clickUI.modeCombo.addItem("Noise",  static_cast<int>(ClickUI::Mode::Noise));
  clickUI.modeCombo.addItem("Sample", static_cast<int>(ClickUI::Mode::Sample));
  clickUI.modeCombo.setSelectedId(static_cast<int>(ClickUI::Mode::Tone),
                                  juce::dontSendNotification);
  clickUI.modeCombo.setLookAndFeel(&darkComboLAF);
  clickUI.modeCombo.onChange = [this] {
    const bool isTone = (clickUI.modeCombo.getSelectedId() ==
                         static_cast<int>(ClickUI::Mode::Tone));
    clickUI.xyPad.setVisible(isTone);
  };
  addAndMakeVisible(clickUI.modeCombo);

  // ── XY Pad ──
  // blend: 0=SQR, 1=SAW → clickToneOsc の波形選択
  // decay: 0=LONG, 1=SHORT → 指数減衰の時定数 (200ms〜5ms)
  clickUI.xyPad.setOnChanged([this](float blend, float decay) {
    processorRef.clickEngine().setBlend(blend);
    const float decayMs = juce::jmap(decay, 0.0f, 1.0f, 200.0f, 5.0f);
    processorRef.clickEngine().setDecayMs(decayMs);
  });
  addAndMakeVisible(clickUI.xyPad);

  // 起動時の初期値を DSP へ反映（コールバックは dragg 時のみ呼ばれるため）
  processorRef.clickEngine().setBlend(clickUI.xyPad.getBlend());
  processorRef.clickEngine().setDecayMs(
      juce::jmap(clickUI.xyPad.getDecay(), 0.0f, 1.0f, 200.0f, 5.0f));

  // ── Filter params (FREQ1 / FOCUS / FREQ2 / FOCUS) ──
  const auto tinyFont = juce::Font(juce::FontOptions(UIConstants::fontSizeSmall));
  styleFilterLabel(clickUI.freq1Label,  "FREQ1:", tinyFont);
  styleFilterLabel(clickUI.focus1Label, "FOCUS:", tinyFont);
  styleFilterLabel(clickUI.freq2Label,  "FREQ2:", tinyFont);
  styleFilterLabel(clickUI.focus2Label, "FOCUS:", tinyFont);
  addAndMakeVisible(clickUI.freq1Label);
  addAndMakeVisible(clickUI.focus1Label);
  addAndMakeVisible(clickUI.freq2Label);
  addAndMakeVisible(clickUI.focus2Label);

  styleFilterBox(clickUI.freq1Box,  "5000",  tinyFont);
  styleFilterBox(clickUI.focus1Box, "0.71",  tinyFont);
  styleFilterBox(clickUI.freq2Box,  "10000", tinyFont);
  styleFilterBox(clickUI.focus2Box, "0",     tinyFont);
  addAndMakeVisible(clickUI.freq1Box);
  addAndMakeVisible(clickUI.focus1Box);
  addAndMakeVisible(clickUI.freq2Box);
  addAndMakeVisible(clickUI.focus2Box);
}

void BabySquatchAudioProcessorEditor::layoutClickParams(
    juce::Rectangle<int> area) {
  // 上段: mode ラベル + コンボ
  auto topRow = area.removeFromTop(22);
  area.removeFromTop(4);
  constexpr int modeLabelW = 38;
  clickUI.modeLabel.setBounds(topRow.removeFromLeft(modeLabelW));
  clickUI.modeCombo.setBounds(topRow);

  // 残りエリア: [XYPad] [4px] [Filter params]
  constexpr int filterPanelW = 84; // label(36) + gap(2) + box(46)
  constexpr int rowGap = 3;
  constexpr int labelW = 36;

  auto filterPanel = area.removeFromRight(filterPanelW);
  area.removeFromRight(4); // XYPad ↔ filterPanel 間のギャップ
  clickUI.xyPad.setBounds(area);

  // filterPanel を 4 行分割
  const int rowH = (filterPanel.getHeight() - rowGap * 3) / 4;
  const std::array<std::pair<juce::Label *, juce::TextEditor *>, 4> rows = {{
      {&clickUI.freq1Label,  &clickUI.freq1Box},
      {&clickUI.focus1Label, &clickUI.focus1Box},
      {&clickUI.freq2Label,  &clickUI.freq2Box},
      {&clickUI.focus2Label, &clickUI.focus2Box},
  }};
  for (auto [label, box] : rows) {
    auto row = filterPanel.removeFromTop(rowH);
    filterPanel.removeFromTop(rowGap);
    label->setBounds(row.removeFromLeft(labelW));
    box->setBounds(row);
  }
}
