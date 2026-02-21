#pragma once

#include "GUI/KeyboardComponent.h"
#include "GUI/PanelComponent.h"
#include "GUI/WaveformDisplay.h"
#include "PluginProcessor.h"

#include <array>

class BabySquatchAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer {
public:
  explicit BabySquatchAudioProcessorEditor(BabySquatchAudioProcessor &);
  ~BabySquatchAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  void timerCallback() override;

  // 展開チャンネル（None = 閉じた状態）
  enum class ExpandChannel { none, oomph, click, dry };

  void requestExpand(ExpandChannel ch);
  void updateExpandIndicators();
  void updateWaveformVisibility();

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
  WaveformDisplay waveformDisplay;
  std::array<float, 1024> waveformTransferBuffer{};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BabySquatchAudioProcessorEditor)
};
