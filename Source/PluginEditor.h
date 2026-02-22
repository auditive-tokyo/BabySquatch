#pragma once

#include "DSP/EnvelopeData.h"
#include "GUI/EnvelopeCurveEditor.h"
#include "GUI/KeyboardComponent.h"
#include "GUI/PanelComponent.h"
#include "PluginProcessor.h"

#include <array>

class BabySquatchAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit BabySquatchAudioProcessorEditor(BabySquatchAudioProcessor &);
  ~BabySquatchAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  // 展開チャンネル（None = 閉じた状態）
  enum class ExpandChannel { none, oomph, click, dry };

  void requestExpand(ExpandChannel ch);
  void updateExpandIndicators();
  void updateEnvelopeEditorVisibility();

  PanelComponent oomphPanel{"OOMPH", UIConstants::Colours::oomphArc,
                            UIConstants::Colours::oomphThumb};

  PanelComponent clickPanel{"CLICK", UIConstants::Colours::clickArc,
                            UIConstants::Colours::clickThumb};

  PanelComponent dryPanel{"DRY", UIConstants::Colours::dryArc,
                          UIConstants::Colours::dryThumb};

  // ── 共有展開エリア（3パネル横断） ──
  juce::Component expandableArea;
  ExpandChannel activeChannel = ExpandChannel::none;

  // ── MIDI 鍵盤（展開パネル下部・全チャンネル共通） ──
  BabySquatchAudioProcessor &processorRef;
  KeyboardComponent keyboard;
  EnvelopeData ampEnvData;
  EnvelopeData pitchEnvData;
  EnvelopeCurveEditor envelopeCurveEditor{ampEnvData, pitchEnvData};

  // ── OOMPH展開パネル: temp ノブ行（8本） ──
  std::array<juce::Slider, 8> tempKnobs;
  std::array<juce::Label, 8> tempKnobLabels;
  // ── ツールチップ（AMPノブの無効時などに使用） ──
  juce::TooltipWindow tooltipWindow{this, 500};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BabySquatchAudioProcessorEditor)
};
