#pragma once

#include "CustomSliderLAF.h"
#include "UIConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>

// ────────────────────────────────────────────────────
// 1 つのパネル = ラベル + 色付きノブ + dB 表示
// ノブ中央の数値をクリックするとキーボード入力可能
// ────────────────────────────────────────────────────
class PanelComponent : public juce::Component,
                       private juce::TextEditor::Listener {
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

    // ── 値クリックエリア（透明、ノブ中央の数値上でクリック検出） ──
    clickArea.onClick = [this] { showValueEditor(); };
    addAndMakeVisible(clickArea);

    // ── 値入力 TextEditor（初期非表示） ──
    valueEditor.setJustification(juce::Justification::centred);
    valueEditor.setColour(juce::TextEditor::backgroundColourId,
                          UIConstants::Colours::knobBg);
    valueEditor.setColour(juce::TextEditor::textColourId,
                          UIConstants::Colours::text);
    valueEditor.setColour(juce::TextEditor::outlineColourId,
                          juce::Colours::white.withAlpha(0.3f));
    valueEditor.setColour(juce::TextEditor::focusedOutlineColourId,
                          juce::Colours::white.withAlpha(0.5f));
    valueEditor.setInputRestrictions(7, "-+.0123456789");
    valueEditor.addListener(this);
    addChildComponent(valueEditor); // 非表示で追加

    // ── グローバルクリック監視（BOX外クリックで閉じる） ──
    globalClickListener.onMouseDown = [this](const juce::MouseEvent &e) {
      if (const auto *src = e.eventComponent;
          src != &valueEditor && !valueEditor.isParentOf(src) &&
          src != &clickArea) {
        hideValueEditor(false);
      }
    };
  }

  ~PanelComponent() override {
    juce::Desktop::getInstance().removeGlobalMouseListener(
        &globalClickListener);
    knob.setLookAndFeel(nullptr);
  }

  void paint(juce::Graphics &g) override {
    g.setColour(UIConstants::Colours::panelBg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
  }

  void resized() override {
    auto area = getLocalBounds().reduced(UIConstants::panelPadding);

    titleLabel.setBounds(area.removeFromTop(UIConstants::labelHeight));
    knob.setBounds(area);

    // ノブ中央の値テキスト領域を算出（CustomSliderLAF と同じ計算）
    const auto knobBounds = knob.getBounds();
    const auto kw = static_cast<float>(knobBounds.getWidth());
    const auto kh = static_cast<float>(knobBounds.getHeight());
    const float radius = juce::jmin(kw, kh) * 0.40f;
    const auto cx = static_cast<float>(knobBounds.getCentreX());
    const auto cy = static_cast<float>(knobBounds.getCentreY());
    const float fontSize = radius * 0.38f;

    auto valueBounds =
        juce::Rectangle<float>(cx - radius * 0.7f, cy - fontSize * 0.5f,
                               radius * 1.4f, fontSize)
            .toNearestInt();

    clickArea.setBounds(valueBounds.expanded(4));
    valueEditor.setBounds(valueBounds.expanded(2, 4));
    valueEditor.setFont(juce::Font(juce::FontOptions(fontSize)));
  }

  juce::Slider &getKnob() { return knob; }

private:
  // ── 透明クリック領域（ノブ中央の値テキスト上に配置） ──
  class ValueClickArea : public juce::Component {
  public:
    std::function<void()> onClick;
    void mouseDown(const juce::MouseEvent & /*event*/) override {
      if (onClick)
        onClick();
    }
  };

  // ── 値エディタ表示 ──
  void showValueEditor() {
    valueEditor.setVisible(true);
    valueEditor.setText(juce::String(knob.getValue(), 1), false);
    valueEditor.selectAll();
    valueEditor.grabKeyboardFocus();
    juce::Desktop::getInstance().addGlobalMouseListener(&globalClickListener);
  }

  // ── 値エディタ非表示 ──
  void hideValueEditor(bool applyValue) {
    if (!valueEditor.isVisible())
      return;

    if (applyValue) {
      const auto text = valueEditor.getText().trim();
      if (text.containsOnly("-+.0123456789") && text.isNotEmpty()) {
        const auto newVal = juce::jlimit(-24.0, 24.0, text.getDoubleValue());
        knob.setValue(newVal, juce::sendNotificationAsync);
      }
    }

    juce::Desktop::getInstance().removeGlobalMouseListener(
        &globalClickListener);
    valueEditor.setVisible(false);
  }

  // ── TextEditor::Listener ──
  void textEditorReturnKeyPressed(juce::TextEditor & /*editor*/) override {
    hideValueEditor(true);
  }

  void textEditorEscapeKeyPressed(juce::TextEditor & /*editor*/) override {
    hideValueEditor(false);
  }

  void textEditorFocusLost(juce::TextEditor & /*editor*/) override {
    hideValueEditor(false);
  }

  // ── グローバルクリックリスナー（BOX外クリック検出用） ──
  struct GlobalClickListener : public juce::MouseListener {
    std::function<void(const juce::MouseEvent &)> onMouseDown;
    void mouseDown(const juce::MouseEvent &event) override {
      if (onMouseDown)
        onMouseDown(event);
    }
  };

  ColouredSliderLAF laf;
  juce::Slider knob;
  juce::Label titleLabel;
  ValueClickArea clickArea;
  juce::TextEditor valueEditor;
  GlobalClickListener globalClickListener;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelComponent)
};
