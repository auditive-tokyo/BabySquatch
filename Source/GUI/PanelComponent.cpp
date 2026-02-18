#include "PanelComponent.h"
#include "UIConstants.h"

// ────────────────────────────────────────────────────
// コンストラクタ
// ────────────────────────────────────────────────────
PanelComponent::PanelComponent(const juce::String &name,
                                juce::Colour arcColour,
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

// ────────────────────────────────────────────────────
// デストラクタ
// ────────────────────────────────────────────────────
PanelComponent::~PanelComponent() {
  juce::Desktop::getInstance().removeGlobalMouseListener(&globalClickListener);
  knob.setLookAndFeel(nullptr);
}

// ────────────────────────────────────────────────────
// paint / resized
// ────────────────────────────────────────────────────
void PanelComponent::paint(juce::Graphics &g) {
  g.setColour(UIConstants::Colours::panelBg);
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
}

void PanelComponent::resized() {
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

// ────────────────────────────────────────────────────
// getKnob
// ────────────────────────────────────────────────────
juce::Slider &PanelComponent::getKnob() { return knob; }

// ────────────────────────────────────────────────────
// ValueClickArea
// ────────────────────────────────────────────────────
void PanelComponent::ValueClickArea::mouseDown(
    const juce::MouseEvent & /*event*/) {
  if (onClick)
    onClick();
}

// ────────────────────────────────────────────────────
// GlobalClickListener
// ────────────────────────────────────────────────────
void PanelComponent::GlobalClickListener::mouseDown(
    const juce::MouseEvent &event) {
  if (onMouseDown)
    onMouseDown(event);
}

// ────────────────────────────────────────────────────
// showValueEditor / hideValueEditor
// ────────────────────────────────────────────────────
void PanelComponent::showValueEditor() {
  valueEditor.setVisible(true);
  valueEditor.setText(juce::String(knob.getValue(), 1), false);
  valueEditor.selectAll();
  valueEditor.grabKeyboardFocus();
  juce::Desktop::getInstance().addGlobalMouseListener(&globalClickListener);
}

void PanelComponent::hideValueEditor(bool applyValue) {
  if (!valueEditor.isVisible())
    return;

  if (applyValue) {
    const auto text = valueEditor.getText().trim();
    if (text.containsOnly("-+.0123456789") && text.isNotEmpty()) {
      const auto newVal = juce::jlimit(-24.0, 24.0, text.getDoubleValue());
      knob.setValue(newVal, juce::sendNotificationAsync);
    }
  }

  juce::Desktop::getInstance().removeGlobalMouseListener(&globalClickListener);
  valueEditor.setVisible(false);
}

// ────────────────────────────────────────────────────
// TextEditor::Listener
// ────────────────────────────────────────────────────
void PanelComponent::textEditorReturnKeyPressed(
    juce::TextEditor & /*editor*/) {
  hideValueEditor(true);
}

void PanelComponent::textEditorEscapeKeyPressed(
    juce::TextEditor & /*editor*/) {
  hideValueEditor(false);
}

void PanelComponent::textEditorFocusLost(juce::TextEditor & /*editor*/) {
  hideValueEditor(false);
}
