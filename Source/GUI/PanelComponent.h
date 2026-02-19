#pragma once

#include "CustomSliderLAF.h"
#include <juce_gui_basics/juce_gui_basics.h>

// ────────────────────────────────────────────────────
// 1 つのパネル = ラベル + 色付きノブ + dB 表示
// ノブ中央の数値をクリックするとキーボード入力可能
// ────────────────────────────────────────────────────
class PanelComponent : public juce::Component,
                       private juce::TextEditor::Listener {
public:
  PanelComponent(const juce::String &name, juce::Colour arcColour,
                 juce::Colour thumbColour);
  ~PanelComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  juce::Slider &getKnob();

  // 展開ボタンが押された時に親へ通知するコールバック
  std::function<void()> onExpandRequested;

  // 展開インジケータ更新（親から呼ばれる）
  void setExpandIndicator(bool isThisPanelExpanded);

private:
  // ── 透明クリック領域（ノブ中央の値テキスト上に配置） ──
  class ValueClickArea : public juce::Component {
  public:
    std::function<void()> onClick;
    void mouseDown(const juce::MouseEvent &event) override;
  };

  // ── グローバルクリックリスナー（BOX外クリック検出用） ──
  struct GlobalClickListener : public juce::MouseListener {
    std::function<void(const juce::MouseEvent &)> onMouseDown;
    void mouseDown(const juce::MouseEvent &event) override;
  };

  void showValueEditor();
  void hideValueEditor(bool applyValue);

  void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
  void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
  void textEditorFocusLost(juce::TextEditor &editor) override;

  ColouredSliderLAF laf;
  juce::Slider knob;
  juce::Label titleLabel;
  ValueClickArea clickArea;
  juce::TextEditor valueEditor;
  GlobalClickListener globalClickListener;
  juce::TextButton expandButton;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelComponent)
};
