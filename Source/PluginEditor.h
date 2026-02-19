#pragma once

#include "GUI/PanelComponent.h"
#include "PluginProcessor.h"

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

  PanelComponent oomphPanel{"OOMPH", UIConstants::Colours::oomphArc,
                            UIConstants::Colours::oomphThumb};

  PanelComponent clickPanel{"CLICK", UIConstants::Colours::clickArc,
                            UIConstants::Colours::clickThumb};

  PanelComponent dryPanel{"DRY", UIConstants::Colours::dryArc,
                          UIConstants::Colours::dryThumb};

  // ── 共有展開エリア（3パネル横断） ──
  juce::Component expandableArea;
  ExpandChannel activeChannel = ExpandChannel::none;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BabySquatchAudioProcessorEditor)
};
