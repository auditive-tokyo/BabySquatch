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
  using enum ChannelState::Channel;
  oomphPanel.setOnMuteChanged(
      [&p](bool m) { p.channelState().setMute(oomph, m); });
  oomphPanel.setOnSoloChanged(
      [&p](bool s) { p.channelState().setSolo(oomph, s); });
  clickPanel.setOnMuteChanged(
      [&p](bool m) { p.channelState().setMute(click, m); });
  clickPanel.setOnSoloChanged(
      [&p](bool s) { p.channelState().setSolo(click, s); });
  dryPanel.setOnMuteChanged([&p](bool m) { p.channelState().setMute(dry, m); });
  dryPanel.setOnSoloChanged([&p](bool s) { p.channelState().setSolo(dry, s); });

  // ── レベルメーター配線 ──
  oomphPanel.setLevelProvider(
      [&p]() { return p.channelState().getChannelLevelDb(oomph); });
  clickPanel.setLevelProvider(
      [&p]() { return p.channelState().getChannelLevelDb(click); });
  dryPanel.setLevelProvider(
      [&p]() { return p.channelState().getChannelLevelDb(dry); });

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

  // ── LUT ベイクヘルパー（AMP） ──
  auto bakeAmpLut = [this] {
    constexpr int lutSize = EnvelopeLutManager::lutSize;
    std::array<float, lutSize> lut{};
    const float durationMs = envelopeCurveEditor.getDisplayDurationMs();
    for (int i = 0; i < lutSize; ++i) {
      const float timeMs =
          static_cast<float>(i) / static_cast<float>(lutSize - 1) * durationMs;
      lut[static_cast<size_t>(i)] = ampEnvData.evaluate(timeMs);
    }
    processorRef.envLut().setDurationMs(durationMs);
    processorRef.envLut().bake(lut.data(), lutSize);
  };

  // ── LUT ベイクヘルパー（Pitch: Hz値を格納） ──
  auto bakePitchLut = [this] {
    constexpr int lutSize = EnvelopeLutManager::lutSize;
    std::array<float, lutSize> lut{};
    const float durationMs = envelopeCurveEditor.getDisplayDurationMs();
    for (int i = 0; i < lutSize; ++i) {
      const float timeMs =
          static_cast<float>(i) / static_cast<float>(lutSize - 1) * durationMs;
      lut[static_cast<size_t>(i)] = pitchEnvData.evaluate(timeMs);
    }
    processorRef.pitchLut().setDurationMs(durationMs);
    processorRef.pitchLut().bake(lut.data(), lutSize);
  };

  // AMP + Pitch 両方をベイク
  auto bakeAllLuts = [bakeAmpLut, bakePitchLut] {
    bakeAmpLut();
    bakePitchLut();
  };

  // ── pitchEnvData のデフォルト値 = 200 Hz ──
  pitchEnvData.setDefaultValue(200.0f);

  // ── ポイント変更 → LUT 再ベイク + ノブ状態更新 ──
  envelopeCurveEditor.setOnChange([this, bakeAllLuts] {
    bakeAllLuts();
    // AMP ノブ: エンベロープポイントがあれば無効化
    const bool ampControlled = ampEnvData.hasPoints();
    oomphKnobs[1].setEnabled(!ampControlled);
    oomphKnobs[1].setTooltip(ampControlled ? "Value is controlled by envelope"
                                           : "");
    // PITCH ノブ: エンベロープポイントがあれば無効化
    const bool pitchControlled = pitchEnvData.hasPoints();
    oomphKnobs[0].setEnabled(!pitchControlled);
    oomphKnobs[0].setTooltip(pitchControlled ? "Value is controlled by envelope"
                                             : "");
  });

  // ── OOMPHノブ → Processorゲイン配線のみ（振幅は AMP ノブで制御） ──
  oomphPanel.getKnob().onValueChange = [this] {
    const auto dB = static_cast<float>(oomphPanel.getKnob().getValue());
    processorRef.setOomphGainDb(dB);
  };

  setSize(UIConstants::windowWidth, UIConstants::windowHeight);

  // ── OOMPH Osc ノブ行（8本）初期化 ──
  for (int i = 0; i < 8; ++i) {
    const auto idx = static_cast<size_t>(i);
    auto &knob = oomphKnobs[idx];
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setRange(0.0, 1.0);
    addChildComponent(knob);

    auto &label = oomphKnobLabels[idx];
    juce::String labelText;
    if (i == 0)
      labelText = "PITCH";
    else if (i == 1)
      labelText = "AMP";
    else if (i == 2)
      labelText = "BLEND";
    else if (i == 3)
      labelText = "DIST";
    else if (i == 4)
      labelText = "H1";
    else if (i == 5)
      labelText = "H2";
    else if (i == 6)
      labelText = "H3";
    else if (i == 7)
      labelText = "H4";
    else
      labelText = "temp " + juce::String(i + 1);
    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(10.0f)));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, UIConstants::Colours::labelText);
    addChildComponent(label);
  }

  // ── PITCH/AMP ラベルクリックで編集対象切替 ──
  // (タブは EnvelopeCurveEditor 内部で描画・クリック処理)
  envelopeCurveEditor.setOnEditTargetChanged(
      [this](EnvelopeCurveEditor::EditTarget target) {
        using enum EnvelopeCurveEditor::EditTarget;
        oomphKnobLabels[0].setColour(juce::Label::textColourId,
                                     target == pitch
                                         ? juce::Colours::cyan
                                         : UIConstants::Colours::labelText);
        oomphKnobLabels[1].setColour(juce::Label::textColourId,
                                     target == amp
                                         ? juce::Colours::white
                                         : UIConstants::Colours::labelText);
      });

  // ── PITCH ノブ（oomphKnobs[0]）: range・初期値・コールバック設定 ──
  // 20〜20000 Hz、対数スケール、デフォルト 200 Hz
  oomphKnobs[0].setRange(20.0, 20000.0);
  oomphKnobs[0].setSkewFactorFromMidPoint(200.0);
  oomphKnobs[0].setValue(200.0, juce::dontSendNotification);
  oomphKnobs[0].setDoubleClickReturnValue(true, 200.0);
  oomphKnobs[0].onValueChange = [this, bakePitchLut] {
    const auto hz = static_cast<float>(oomphKnobs[0].getValue());
    // pitchEnvData のデフォルト値を更新
    pitchEnvData.setDefaultValue(hz);
    bakePitchLut();
    // 波形プレビューのサイクル数を更新
    const float cycles =
        hz * envelopeCurveEditor.getDisplayDurationMs() / 1000.0f;
    envelopeCurveEditor.setDisplayCycles(cycles);
    envelopeCurveEditor.repaint();
  };
  // 初期サイクル数 + Pitch LUT 初期ベイク
  {
    const float initHz = 200.0f;
    envelopeCurveEditor.setDisplayCycles(
        initHz * envelopeCurveEditor.getDisplayDurationMs() / 1000.0f);
    bakePitchLut();
  }

  // ── AMP ノブ（oomphKnobs[1]）: range・初期値・コールバック設定 ──
  // 表示値は 0〜200％、内部変換: gain = value / 100.0f
  oomphKnobs[1].setRange(0.0, 200.0);
  oomphKnobs[1].setValue(ampEnvData.getDefaultValue() * 100.0,
                         juce::dontSendNotification);
  oomphKnobs[1].setDoubleClickReturnValue(true, 100.0);
  oomphKnobs[1].onValueChange = [this, bakeAmpLut] {
    const auto gain = static_cast<float>(oomphKnobs[1].getValue()) / 100.0f;
    ampEnvData.setDefaultValue(gain);
    bakeAmpLut();
    envelopeCurveEditor.repaint();
  };
  // 初期状態: ポイントがあれば無効化
  {
    const bool controlled = ampEnvData.hasPoints();
    oomphKnobs[1].setEnabled(!controlled);
    oomphKnobs[1].setTooltip(controlled ? "Value is controlled by envelope"
                                        : "");
  }

  // ── H1〜H4 ノブ（oomphKnobs[4〜7]）: range・コールバック設定 ──
  for (int i = 0; i < 4; ++i) {
    const auto idx = static_cast<size_t>(i + 4); // knobs[4]〜[7]
    oomphKnobs[idx].setRange(0.0, 1.0, 0.01);
    oomphKnobs[idx].setValue(0.0, juce::dontSendNotification);
    oomphKnobs[idx].setDoubleClickReturnValue(true, 0.0);
    const int harmonicNum = i + 1; // H1〜H4
    oomphKnobs[idx].onValueChange = [this, idx, harmonicNum] {
      processorRef.oomphOscillator().setHarmonicGain(
          harmonicNum, static_cast<float>(oomphKnobs[idx].getValue()));
    };
  }
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

    // 2. 全チャンネル共通: パラメータノブ行のスペースを上部から確保
    //    （OOMPH以外ではノブは非表示だが、波形エリアの高さを揃えるため常に確保）
    {
      auto knobRow = expandArea.removeFromTop(UIConstants::oomphKnobRowHeight);
      if (activeChannel == oomph) {
        const int slotW = knobRow.getWidth() / 8;
        for (int i = 0; i < 8; ++i) {
          const auto idx = static_cast<size_t>(i);
          auto slot = knobRow.removeFromLeft(slotW);
          oomphKnobLabels[idx].setBounds(slot.removeFromBottom(18));
          oomphKnobs[idx].setBounds(slot);
        }
      }
    }

    // 3. エンベロープカーブエディタ → 残り全域
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

  // setSize でサイズが変わらない場合（チャンネル切替）は resized() が
  // 自動呼出しされないため、明示的にレイアウトを更新
  resized();

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

  const bool oomphOpen = (activeChannel == oomph);
  for (size_t i = 0; i < 8; ++i) {
    oomphKnobs[i].setVisible(oomphOpen);
    oomphKnobLabels[i].setVisible(oomphOpen);
  }
}
