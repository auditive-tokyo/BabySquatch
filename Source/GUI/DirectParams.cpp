// DirectParams.cpp
// Direct panel UI setup / layout
#include "../PluginEditor.h"

namespace {
void styleDirectKnob(juce::Slider &s, ColouredSliderLAF &laf) {
  s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  s.setLookAndFeel(&laf);
}
void styleKnobLabelDirect(juce::Label &label, const juce::String &text,
                          const juce::Font &font) {
  label.setText(text, juce::dontSendNotification);
  label.setFont(font);
  label.setColour(juce::Label::textColourId, UIConstants::Colours::labelText);
  label.setJustificationType(juce::Justification::centred);
}
} // namespace

static std::pair<float, float> computeDirectPreview(
    const std::vector<float> &thumbMin, const std::vector<float> &thumbMax,
    double durSec, float attackSec, float holdSec, float releaseSec,
    float timeSec) {
  // A → Hold → R エンベロープ
  float env = 0.0f;
  if (timeSec < attackSec) {
    env = (attackSec > 0.0f) ? timeSec / attackSec : 1.0f;
  } else if (timeSec < attackSec + holdSec) {
    env = 1.0f;
  } else {
    const float rel = timeSec - attackSec - holdSec;
    env = (releaseSec > 0.0f)
              ? juce::jlimit(0.0f, 1.0f, 1.0f - rel / releaseSec)
              : 0.0f;
  }
  // サムネイル参照
  if (durSec <= 0.0 || timeSec < 0.0f || timeSec >= static_cast<float>(durSec))
    return {0.0f, 0.0f};
  const float t = timeSec / static_cast<float>(durSec);
  const auto n = static_cast<int>(thumbMin.size());
  const int idx =
      juce::jlimit(0, n - 1, static_cast<int>(t * static_cast<float>(n)));
  return {thumbMin[static_cast<std::size_t>(idx)] * env,
          thumbMax[static_cast<std::size_t>(idx)] * env};
}

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

  // ── Sample load button ──
  directUI.sampleLoadButton.setColour(
      juce::TextButton::buttonColourId,
      UIConstants::Colours::panelBg.brighter(0.15f));
  directUI.sampleLoadButton.setColour(juce::TextButton::textColourOffId,
                                      UIConstants::Colours::labelText);
  directUI.sampleLoadButton.setVisible(false);
  directUI.sampleLoadButton.onClick = [this] { onSampleLoadClicked(); };
  directUI.sampleLoadButton.setOnFileDropped(
      [this](const juce::File &file) { onSampleFileChosen(file); });
  addAndMakeVisible(directUI.sampleLoadButton);

  // ── Mode コンボ変更時: ボタン表示切り替え ──
  directUI.modeCombo.onChange = [this] {
    const bool isSample = directUI.modeCombo.getSelectedId() ==
                          static_cast<int>(DirectUI::Mode::Sample);
    directUI.sampleLoadButton.setVisible(isSample);
    if (!isSample)
      directUI.sampleLoadButton.setButtonText("Drop or Click to Load");
    resized();
  };
  // ── 8 ノブ セットアップ ──
  const auto knobFont =
      juce::Font(juce::FontOptions(UIConstants::fontSizeSmall));

  // Pitch: -24 〜 +24 半音
  styleDirectKnob(directUI.pitchSlider, directKnobLAF);
  directUI.pitchSlider.setRange(-24.0, 24.0, 1.0);
  directUI.pitchSlider.setDoubleClickReturnValue(true, 0.0);
  directUI.pitchSlider.setValue(0.0, juce::dontSendNotification);
  directUI.pitchSlider.onValueChange = [this] {
    processorRef.directEngine().setPitchSemitones(
        static_cast<float>(directUI.pitchSlider.getValue()));
    refreshDirectProvider();
  };
  addAndMakeVisible(directUI.pitchSlider);
  styleKnobLabelDirect(directUI.pitchLabel, "Pitch", knobFont);
  addAndMakeVisible(directUI.pitchLabel);

  // Attack: 0.1 〜 500 ms
  styleDirectKnob(directUI.attackSlider, directKnobLAF);
  directUI.attackSlider.setRange(0.1, 500.0, 0.1);
  directUI.attackSlider.setSkewFactorFromMidPoint(20.0);
  directUI.attackSlider.setDoubleClickReturnValue(true, 1.0);
  directUI.attackSlider.setValue(1.0, juce::dontSendNotification);
  directUI.attackSlider.onValueChange = [this] {
    processorRef.directEngine().setAttackMs(
        static_cast<float>(directUI.attackSlider.getValue()));
    refreshDirectProvider();
  };
  addAndMakeVisible(directUI.attackSlider);
  styleKnobLabelDirect(directUI.attackLabel, "A", knobFont);
  addAndMakeVisible(directUI.attackLabel);

  // Decay: 1 〜 2000 ms
  styleDirectKnob(directUI.decaySlider, directKnobLAF);
  directUI.decaySlider.setRange(1.0, 2000.0, 1.0);
  directUI.decaySlider.setSkewFactorFromMidPoint(200.0);
  directUI.decaySlider.setDoubleClickReturnValue(true, 200.0);
  directUI.decaySlider.setValue(200.0, juce::dontSendNotification);
  directUI.decaySlider.onValueChange = [this] {
    processorRef.directEngine().setDecayMs(
        static_cast<float>(directUI.decaySlider.getValue()));
    refreshDirectProvider();
  };
  addAndMakeVisible(directUI.decaySlider);
  styleKnobLabelDirect(directUI.decayLabel, "D", knobFont);
  addAndMakeVisible(directUI.decayLabel);

  // Release: 1 〜 1000 ms
  styleDirectKnob(directUI.releaseSlider, directKnobLAF);
  directUI.releaseSlider.setRange(1.0, 1000.0, 1.0);
  directUI.releaseSlider.setSkewFactorFromMidPoint(100.0);
  directUI.releaseSlider.setDoubleClickReturnValue(true, 50.0);
  directUI.releaseSlider.setValue(50.0, juce::dontSendNotification);
  directUI.releaseSlider.onValueChange = [this] {
    processorRef.directEngine().setReleaseMs(
        static_cast<float>(directUI.releaseSlider.getValue()));
    refreshDirectProvider();
  };
  addAndMakeVisible(directUI.releaseSlider);
  styleKnobLabelDirect(directUI.releaseLabel, "R", knobFont);
  addAndMakeVisible(directUI.releaseLabel);

  // HPF: SlopeSelector (label) + freq knob + Q knob
  directUI.hpfSlope.setOnChange(
      [this](int dboct) { processorRef.directEngine().setHpfSlope(dboct); });
  addAndMakeVisible(directUI.hpfSlope);

  styleDirectKnob(directUI.hpfSlider, directKnobLAF);
  directUI.hpfSlider.setRange(20.0, 20000.0, 1.0);
  directUI.hpfSlider.setSkewFactorFromMidPoint(1000.0);
  directUI.hpfSlider.setTextValueSuffix(" Hz");
  directUI.hpfSlider.setValue(20.0, juce::dontSendNotification);
  directUI.hpfSlider.setDoubleClickReturnValue(true, 20.0);
  directUI.hpfSlider.onValueChange = [this] {
    processorRef.directEngine().setHpfFreq(
        static_cast<float>(directUI.hpfSlider.getValue()));
  };
  addAndMakeVisible(directUI.hpfSlider);

  styleDirectKnob(directUI.hpfQSlider, directKnobLAF);
  directUI.hpfQSlider.setRange(0.1, 12.0, 0.01);
  directUI.hpfQSlider.textFromValueFunction = [](double v) {
    return juce::String(v, 2);
  };
  directUI.hpfQSlider.setValue(0.707, juce::dontSendNotification);
  directUI.hpfQSlider.setDoubleClickReturnValue(true, 0.707);
  directUI.hpfQSlider.onValueChange = [this] {
    processorRef.directEngine().setHpfQ(
        static_cast<float>(directUI.hpfQSlider.getValue()));
  };
  addAndMakeVisible(directUI.hpfQSlider);
  styleKnobLabelDirect(directUI.hpfQLabel, "Q", knobFont);
  addAndMakeVisible(directUI.hpfQLabel);

  // LPF: SlopeSelector (label) + freq knob + Q knob
  directUI.lpfSlope.setOnChange(
      [this](int dboct) { processorRef.directEngine().setLpfSlope(dboct); });
  addAndMakeVisible(directUI.lpfSlope);

  styleDirectKnob(directUI.lpfSlider, directKnobLAF);
  directUI.lpfSlider.setRange(20.0, 20000.0, 1.0);
  directUI.lpfSlider.setSkewFactorFromMidPoint(1000.0);
  directUI.lpfSlider.setTextValueSuffix(" Hz");
  directUI.lpfSlider.setValue(20000.0, juce::dontSendNotification);
  directUI.lpfSlider.setDoubleClickReturnValue(true, 20000.0);
  directUI.lpfSlider.onValueChange = [this] {
    processorRef.directEngine().setLpfFreq(
        static_cast<float>(directUI.lpfSlider.getValue()));
  };
  addAndMakeVisible(directUI.lpfSlider);

  styleDirectKnob(directUI.lpfQSlider, directKnobLAF);
  directUI.lpfQSlider.setRange(0.1, 12.0, 0.01);
  directUI.lpfQSlider.textFromValueFunction = [](double v) {
    return juce::String(v, 2);
  };
  directUI.lpfQSlider.setValue(0.707, juce::dontSendNotification);
  directUI.lpfQSlider.setDoubleClickReturnValue(true, 0.707);
  directUI.lpfQSlider.onValueChange = [this] {
    processorRef.directEngine().setLpfQ(
        static_cast<float>(directUI.lpfQSlider.getValue()));
  };
  addAndMakeVisible(directUI.lpfQSlider);
  styleKnobLabelDirect(directUI.lpfQLabel, "Q", knobFont);
  addAndMakeVisible(directUI.lpfQLabel);

  // ── 起動時デフォルト値を DSP へ反映 ──
  processorRef.directEngine().setPitchSemitones(0.0f);
  processorRef.directEngine().setAttackMs(1.0f);
  processorRef.directEngine().setDecayMs(200.0f);
  processorRef.directEngine().setReleaseMs(50.0f);
  processorRef.directEngine().setHpfFreq(20.0f); // 20Hz = バイパス
  processorRef.directEngine().setHpfQ(0.707f);
  processorRef.directEngine().setHpfSlope(12);
  processorRef.directEngine().setLpfFreq(20000.0f); // 20kHz = バイパス
  processorRef.directEngine().setLpfQ(0.707f);
  processorRef.directEngine().setLpfSlope(12);
}

void BabySquatchAudioProcessorEditor::layoutDirectParams(
    juce::Rectangle<int> area) {
  // 上段: mode ラベル + コンボ [+ サンプルロードボタン]
  auto topRow = area.removeFromTop(22);
  area.removeFromTop(4);
  constexpr int modeLabelW = 38;
  constexpr int modeComboW = 68;
  directUI.modeLabel.setBounds(topRow.removeFromLeft(modeLabelW));
  topRow.removeFromLeft(2);
  directUI.modeCombo.setBounds(topRow.removeFromLeft(modeComboW));

  if (const bool isSample = directUI.modeCombo.getSelectedId() ==
                            static_cast<int>(DirectUI::Mode::Sample);
      isSample) {
    topRow.removeFromLeft(4);
    directUI.sampleLoadButton.setBounds(topRow);
  }

  // 残りエリア: 2 行 × 4 列グリッド
  const int slotW = area.getWidth() / 4;
  const int rowH = area.getHeight() / 2;

  // 上段行: Pitch / A / D / R
  {
    const std::array<juce::Slider *, 4> rowKnobs = {{
        &directUI.pitchSlider,
        &directUI.attackSlider,
        &directUI.decaySlider,
        &directUI.releaseSlider,
    }};
    const std::array<juce::Label *, 4> rowLabels = {{
        &directUI.pitchLabel,
        &directUI.attackLabel,
        &directUI.decayLabel,
        &directUI.releaseLabel,
    }};
    for (int col = 0; col < 4; ++col) {
      const auto col_u = static_cast<std::size_t>(col);
      juce::Rectangle slot(area.getX() + col * slotW, area.getY(), slotW, rowH);
      rowLabels[col_u]->setBounds(slot.removeFromBottom(14));
      rowKnobs[col_u]->setBounds(slot);
    }
  }

  // 下段行: HP (slope label + freq knob) | Q | LP (slope label + freq knob)
  // | Q
  //   Click と同パターン: SlopeSelector はノブのラベル位置を占める
  {
    const std::array<juce::Slider *, 4> rowKnobs = {{
        &directUI.hpfSlider,
        &directUI.hpfQSlider,
        &directUI.lpfSlider,
        &directUI.lpfQSlider,
    }};
    const std::array<juce::Component *, 4> rowLabels = {{
        &directUI.hpfSlope,
        &directUI.hpfQLabel,
        &directUI.lpfSlope,
        &directUI.lpfQLabel,
    }};
    for (int col = 0; col < 4; ++col) {
      const auto col_u = static_cast<std::size_t>(col);
      juce::Rectangle slot(area.getX() + col * slotW, area.getY() + rowH, slotW,
                           rowH);
      rowLabels[col_u]->setBounds(slot.removeFromBottom(14));
      rowKnobs[col_u]->setBounds(slot);
    }
  }
}

void BabySquatchAudioProcessorEditor::onSampleLoadClicked() {
  directUI.fileChooser = std::make_unique<juce::FileChooser>(
      "Load Sample",
      juce::File::getSpecialLocation(juce::File::userMusicDirectory),
      "*.wav;*.aif;*.aiff;*.flac;*.ogg");
  directUI.fileChooser->launchAsync(
      juce::FileBrowserComponent::openMode |
          juce::FileBrowserComponent::canSelectFiles,
      [this](const juce::FileChooser &fc) {
        const auto result = fc.getResult();
        if (result.existsAsFile())
          onSampleFileChosen(result);
      });
}

void BabySquatchAudioProcessorEditor::onSampleFileChosen(
    const juce::File &file) {
  directUI.loadedFilePath = file.getFullPathName();
  directUI.sampleLoadButton.setButtonText(file.getFileNameWithoutExtension());
  directUI.sampleLoadButton.setTooltip(directUI.loadedFilePath);
  processorRef.directEngine().loadSample(file);

  // サムネイルデータをメンバーに保存してプロバイダーを登録
  if (!processorRef.directEngine().copyWaveformThumbnail(directUI.thumbMin,
                                                         directUI.thumbMax))
    return;
  directUI.thumbDurSec = processorRef.directEngine().getSampleDurationSec();
  refreshDirectProvider();
}

void BabySquatchAudioProcessorEditor::refreshDirectProvider() {
  if (directUI.thumbMin.empty() || directUI.thumbDurSec <= 0.0)
    return;

  // Pitch (semitones) → 再生速度倍率
  const auto semitones = static_cast<float>(directUI.pitchSlider.getValue());
  const float speedRatio = std::pow(2.0f, semitones / 12.0f);
  const double durSec = directUI.thumbDurSec / static_cast<double>(speedRatio);

  // A / D(Hold) / R を秒単位でキャプチャ
  const float attackSec  = static_cast<float>(directUI.attackSlider.getValue())  / 1000.0f;
  const float holdSec    = static_cast<float>(directUI.decaySlider.getValue())   / 1000.0f;
  const float releaseSec = static_cast<float>(directUI.releaseSlider.getValue()) / 1000.0f;

  auto minPtr = std::make_shared<std::vector<float>>(directUI.thumbMin);
  auto maxPtr = std::make_shared<std::vector<float>>(directUI.thumbMax);

  envelopeCurveEditor.setDirectProvider(
      [minPtr, maxPtr, durSec,
       attackSec, holdSec, releaseSec](float timeSec) {
        return computeDirectPreview(*minPtr, *maxPtr, durSec,
                                   attackSec, holdSec, releaseSec, timeSec);
      });
}
