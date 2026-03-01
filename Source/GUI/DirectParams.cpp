// DirectParams.cpp
// Direct panel UI setup / layout
#include "../PluginEditor.h"

void BabySquatchAudioProcessorEditor::setupDirectParams() {
  const auto smallFont =
      juce::Font(juce::FontOptions(UIConstants::fontSizeMedium));

  // ── Mode label ──
  directUI.modeLabel.setText("mode:", juce::dontSendNotification);
  directUI.modeLabel.setFont(smallFont);
  directUI.modeLabel.setColour(juce::Label::textColourId,
                               UIConstants::Colours::labelText);
  directUI.modeLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(directUI.modeLabel);

  // ── Mode combo ──
  directUI.modeCombo.addItem("Direct",
                             static_cast<int>(DirectUI::Mode::Direct));
  directUI.modeCombo.addItem("Sample",
                             static_cast<int>(DirectUI::Mode::Sample));
  directUI.modeCombo.setSelectedId(static_cast<int>(DirectUI::Mode::Direct),
                                   juce::dontSendNotification);
  directUI.modeCombo.setLookAndFeel(&darkComboLAF);
  addAndMakeVisible(directUI.modeCombo);
}

void BabySquatchAudioProcessorEditor::layoutDirectParams(
    juce::Rectangle<int> area) {
  // 上段: mode ラベル + コンボ
  auto topRow = area.removeFromTop(22);
  constexpr int modeLabelW = 38;
  directUI.modeLabel.setBounds(topRow.removeFromLeft(modeLabelW));
  directUI.modeCombo.setBounds(topRow);
}
