// ClickParams.cpp
// Click panel UI setup / layout
#include "../PluginEditor.h"

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
  clickUI.xyPad.setOnChanged([](float /*blend*/, float /*decay*/) {
    // TODO: DSP write (blend -> SQR<->SAW, decay -> Decay ms)
  });
  addAndMakeVisible(clickUI.xyPad);
}

void BabySquatchAudioProcessorEditor::layoutClickParams(
    juce::Rectangle<int> area) {
  // 上段: mode ラベル + コンボ
  auto topRow = area.removeFromTop(22);
  area.removeFromTop(4);
  constexpr int modeLabelW = 38;
  clickUI.modeLabel.setBounds(topRow.removeFromLeft(modeLabelW));
  clickUI.modeCombo.setBounds(topRow);

  // 残りエリア: XYPad（Tone モード時のみ表示）
  clickUI.xyPad.setBounds(area);
}
