#include "PluginEditor.h"
#include "PluginProcessor.h"

BabySquatchAudioProcessorEditor::BabySquatchAudioProcessorEditor(
    BabySquatchAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
      keyboard(p.getKeyboardState()) {
  addAndMakeVisible(oomphPanel);
  addAndMakeVisible(clickPanel);
  addAndMakeVisible(dryPanel);

  // 各パネルの展開ボタン → 親に通知
  oomphPanel.setOnExpandRequested(
      [this] { requestExpand(ExpandChannel::oomph); });
  clickPanel.setOnExpandRequested(
      [this] { requestExpand(ExpandChannel::click); });
  dryPanel.setOnExpandRequested([this] { requestExpand(ExpandChannel::dry); });

  // ── Mute / Solo 配線 ──
  using enum BabySquatchAudioProcessor::Channel;
  oomphPanel.setOnMuteChanged([&p](bool m) { p.setMute(oomph, m); });
  oomphPanel.setOnSoloChanged([&p](bool s) { p.setSolo(oomph, s); });
  clickPanel.setOnMuteChanged([&p](bool m) { p.setMute(click, m); });
  clickPanel.setOnSoloChanged([&p](bool s) { p.setSolo(click, s); });
  dryPanel.setOnMuteChanged([&p](bool m)   { p.setMute(dry,   m); });
  dryPanel.setOnSoloChanged([&p](bool s)   { p.setSolo(dry,   s); });

  // ── 展開エリア（初期非表示） ──
  addChildComponent(expandableArea);

  // ── MIDI 鍵盤（展開パネル下部） ──
  keyboard.setOnModeChanged([this, &p](KeyboardComponent::Mode mode) {
    p.setFixedModeActive(mode == KeyboardComponent::Mode::fixed);
    updateEnvelopeEditorVisibility();
  });
  addChildComponent(keyboard);

  // ── エンベロープカーブエディタ（展開パネル） ──
  addChildComponent(envelopeCurveEditor);

  // ── LUT ベイクヘルパー ──
  auto bakeLut = [this] {
    constexpr int lutSize = BabySquatchAudioProcessor::envLutSize;
    std::array<float, lutSize> lut{};
    const float durationMs = envelopeCurveEditor.getDisplayDurationMs();
    for (int i = 0; i < lutSize; ++i) {
      const float timeMs =
          static_cast<float>(i) / static_cast<float>(lutSize - 1) * durationMs;
      lut[static_cast<size_t>(i)] = ampEnvData.evaluate(timeMs);
    }
    processorRef.setEnvDurationMs(durationMs);
    processorRef.bakeEnvelopeLut(lut.data(), lutSize);
  };

  // ── ポイント変更 → LUT 再ベイク ──
  envelopeCurveEditor.setOnChange(bakeLut);

  // ── OOMPHノブ → Processorゲイン配線 + エンベロープ連動 + LUT ベイク ──
  oomphPanel.getKnob().onValueChange = [this, bakeLut] {
    const auto dB = static_cast<float>(oomphPanel.getKnob().getValue());
    processorRef.setOomphGainDb(dB);
    const float gain = juce::Decibels::decibelsToGain(dB);
    ampEnvData.setDefaultValue(gain);
    bakeLut();
    envelopeCurveEditor.repaint();
  };

  setSize(UIConstants::windowWidth, UIConstants::windowHeight);
}

BabySquatchAudioProcessorEditor::~BabySquatchAudioProcessorEditor() = default;

void BabySquatchAudioProcessorEditor::paint(juce::Graphics &g) {
  using enum ExpandChannel;
  g.fillAll(UIConstants::Colours::background);

  // 展開エリアの背景
  if (activeChannel != none) {
    g.setColour(UIConstants::Colours::panelBg);
    g.fillRoundedRectangle(expandableArea.getBounds().toFloat(), 6.0f);
  }
}

void BabySquatchAudioProcessorEditor::resized() {
  using enum ExpandChannel;
  auto area = getLocalBounds().reduced(UIConstants::panelPadding);

  // 展開エリアを下から確保
  if (activeChannel != none) {
    auto expandArea = area.removeFromBottom(UIConstants::expandedAreaHeight);

    // 1. 鍵盤を展開エリア下部に配置（モードボタン + 鍵盤本体）
    keyboard.setBounds(expandArea.removeFromBottom(
        UIConstants::keyboardHeight + UIConstants::modeButtonHeight));

    // 2. エンベロープカーブエディタ → 残り全域
    envelopeCurveEditor.setBounds(expandArea);
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
  using enum ExpandChannel;
  // 同じチャンネルをもう一度押したら閉じる
  if (activeChannel == ch) {
    activeChannel = none;
  } else {
    activeChannel = ch;
  }

  const bool isOpen = (activeChannel != none);
  expandableArea.setVisible(isOpen);
  keyboard.setVisible(isOpen);
  updateEnvelopeEditorVisibility();

  if (isOpen)
    keyboard.grabFocus();

  const int extra =
      isOpen ? UIConstants::expandedAreaHeight + UIConstants::panelGap : 0;
  setSize(UIConstants::windowWidth, UIConstants::windowHeight + extra);

  updateExpandIndicators();
}

void BabySquatchAudioProcessorEditor::updateExpandIndicators() {
  using enum ExpandChannel;
  oomphPanel.setExpandIndicator(activeChannel == oomph);
  clickPanel.setExpandIndicator(activeChannel == click);
  dryPanel.setExpandIndicator(activeChannel == dry);
}

void BabySquatchAudioProcessorEditor::updateEnvelopeEditorVisibility() {
  using enum ExpandChannel;
  const bool isOpen = (activeChannel != none);
  envelopeCurveEditor.setVisible(isOpen);
}
