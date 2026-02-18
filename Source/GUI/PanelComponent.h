#pragma once

#include "CustomSliderLAF.h"
#include "UIConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>

// ────────────────────────────────────────────────────
// 1 つのパネル = ラベル + 色付きノブ + dB 表示
// ────────────────────────────────────────────────────
class PanelComponent : public juce::Component {
public:
  PanelComponent(const juce::String &name, juce::Colour arcColour,
                 juce::Colour thumbColour)
      : laf(arcColour, thumbColour) {
    // ── タイトルラベル ──
    titleLabel.setText(name, juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, UIConstants::Colours::text);
    addAndMakeVisible(titleLabel);

    // ── ノブ ──
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    knob.setRange(-24.0, 24.0, 0.1);
    knob.setValue(0.0);

    // ダブルクリックで 0.0 dB にリセット（JUCE の組み込み機能）
    knob.setDoubleClickReturnValue(true, 0.0);

    knob.setLookAndFeel(&laf);
    addAndMakeVisible(knob);
  }

  ~PanelComponent() override { knob.setLookAndFeel(nullptr); }

  void paint(juce::Graphics &g) override {
    g.setColour(UIConstants::Colours::panelBg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
  }

  void resized() override {
    auto area = getLocalBounds().reduced(UIConstants::panelPadding);

    titleLabel.setBounds(area.removeFromTop(UIConstants::labelHeight));
    knob.setBounds(area);
  }

  juce::Slider &getKnob() { return knob; }

private:
  ColouredSliderLAF laf;
  juce::Slider knob;
  juce::Label titleLabel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelComponent)
};
