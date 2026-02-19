#include "PluginEditor.h"
#include "PluginProcessor.h"

BabySquatchAudioProcessorEditor::BabySquatchAudioProcessorEditor(
    BabySquatchAudioProcessor &p)
    : AudioProcessorEditor(&p) {
  addAndMakeVisible(oomphPanel);
  addAndMakeVisible(clickPanel);
  addAndMakeVisible(dryPanel);

  // 各パネルの展開ボタン → 親に通知
  oomphPanel.onExpandRequested = [this] {
    requestExpand(ExpandChannel::oomph);
  };
  clickPanel.onExpandRequested = [this] {
    requestExpand(ExpandChannel::click);
  };
  dryPanel.onExpandRequested = [this] { requestExpand(ExpandChannel::dry); };

  // ── 展開エリア（初期非表示） ──
  addChildComponent(expandableArea);

  setSize(UIConstants::windowWidth, UIConstants::windowHeight);
}

BabySquatchAudioProcessorEditor::~BabySquatchAudioProcessorEditor() = default;

void BabySquatchAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(UIConstants::Colours::background);

  // 展開エリアの背景
  if (activeChannel != ExpandChannel::none) {
    g.setColour(UIConstants::Colours::panelBg);
    g.fillRoundedRectangle(expandableArea.getBounds().toFloat(), 6.0f);
  }
}

void BabySquatchAudioProcessorEditor::resized() {
  auto area = getLocalBounds().reduced(UIConstants::panelPadding);

  // 展開エリアを下から確保
  if (activeChannel != ExpandChannel::none) {
    expandableArea.setBounds(
        area.removeFromBottom(UIConstants::expandedAreaHeight));
    area.removeFromBottom(UIConstants::panelGap);
  }

  // 残りを3パネルで均等分割
  const int panelWidth = (area.getWidth() - UIConstants::panelGap * 2) / 3;

  oomphPanel.setBounds(area.removeFromLeft(panelWidth));
  area.removeFromLeft(UIConstants::panelGap);

  clickPanel.setBounds(area.removeFromLeft(panelWidth));
  area.removeFromLeft(UIConstants::panelGap);

  dryPanel.setBounds(area);
}

void BabySquatchAudioProcessorEditor::requestExpand(ExpandChannel ch) {
  // 同じチャンネルをもう一度押したら閉じる
  if (activeChannel == ch) {
    activeChannel = ExpandChannel::none;
  } else {
    activeChannel = ch;
  }

  const bool isOpen = (activeChannel != ExpandChannel::none);
  expandableArea.setVisible(isOpen);

  const int extra =
      isOpen ? UIConstants::expandedAreaHeight + UIConstants::panelGap : 0;
  setSize(UIConstants::windowWidth, UIConstants::windowHeight + extra);

  updateExpandIndicators();
}

void BabySquatchAudioProcessorEditor::updateExpandIndicators() {
  oomphPanel.setExpandIndicator(activeChannel == ExpandChannel::oomph);
  clickPanel.setExpandIndicator(activeChannel == ExpandChannel::click);
  dryPanel.setExpandIndicator(activeChannel == ExpandChannel::dry);
}
