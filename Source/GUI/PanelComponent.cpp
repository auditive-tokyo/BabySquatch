#include "PanelComponent.h"
#include "UIConstants.h"

// ────────────────────────────────────────────────────
// コンストラクタ
// ────────────────────────────────────────────────────
PanelComponent::PanelComponent(const juce::String &name, juce::Colour arcColour,
                               juce::Colour thumbColour)
    : laf(arcColour, thumbColour) {
  // ── レベルメーター ──
  addAndMakeVisible(levelMeter);

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
    if (const auto *src = e.eventComponent; src != &valueEditor &&
                                            !valueEditor.isParentOf(src) &&
                                            src != &clickArea) {
      hideValueEditor(false);
    }
  };

  // ── 展開ボタン ──
  expandButton.setButtonText(juce::String::charToString(0x25BC)); // ▼
  expandButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colours::transparentBlack);
  expandButton.setColour(juce::TextButton::textColourOffId,
                         UIConstants::Colours::labelText);
  expandButton.onClick = [this] {
    if (onExpandRequested)
      onExpandRequested();
  };
  addAndMakeVisible(expandButton);

  // ── Mute ボタン ──
  muteButton.setClickingTogglesState(true);
  muteButton.setColour(juce::TextButton::buttonColourId,
                       UIConstants::Colours::muteOff);
  muteButton.setColour(juce::TextButton::buttonOnColourId,
                       UIConstants::Colours::muteOn);
  muteButton.setColour(juce::TextButton::textColourOffId,
                       UIConstants::Colours::labelText);
  muteButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
  muteButton.onClick = [this] {
    if (onMuteChanged)
      onMuteChanged(muteButton.getToggleState());
  };
  addAndMakeVisible(muteButton);

  // ── Solo ボタン ──
  soloButton.setClickingTogglesState(true);
  soloButton.setColour(juce::TextButton::buttonColourId,
                       UIConstants::Colours::soloOff);
  soloButton.setColour(juce::TextButton::buttonOnColourId,
                       UIConstants::Colours::soloOn);
  soloButton.setColour(juce::TextButton::textColourOffId,
                       UIConstants::Colours::labelText);
  soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
  soloButton.onClick = [this] {
    if (onSoloChanged)
      onSoloChanged(soloButton.getToggleState());
  };
  addAndMakeVisible(soloButton);
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

  // 下部ボタン行: M | S | ▼
  auto bottomRow = area.removeFromBottom(UIConstants::expandButtonHeight);
  const int btnW = bottomRow.getWidth() / 3;
  muteButton.setBounds(bottomRow.removeFromLeft(btnW));
  soloButton.setBounds(bottomRow.removeFromLeft(btnW));
  expandButton.setBounds(bottomRow);

  // ノブ左にレベルメーターを配置
  levelMeter.setBounds(area.removeFromLeft(UIConstants::meterWidth));

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
void PanelComponent::textEditorReturnKeyPressed(juce::TextEditor & /*editor*/) {
  hideValueEditor(true);
}

void PanelComponent::textEditorEscapeKeyPressed(juce::TextEditor & /*editor*/) {
  hideValueEditor(false);
}

void PanelComponent::textEditorFocusLost(juce::TextEditor & /*editor*/) {
  hideValueEditor(false);
}

// ────────────────────────────────────────────────
// setOnExpandRequested
// ────────────────────────────────────────────────
void PanelComponent::setOnExpandRequested(std::function<void()> callback) {
  onExpandRequested = std::move(callback);
}

// ────────────────────────────────────────────────
// setExpandIndicator
// ────────────────────────────────────────────────
void PanelComponent::setExpandIndicator(bool isThisPanelExpanded) {
  expandButton.setButtonText(
      juce::String::charToString(isThisPanelExpanded ? 0x25B2 : 0x25BC));
}

// ────────────────────────────────────────────────
// Mute / Solo コールバック + 外部状態更新
// ────────────────────────────────────────────────
void PanelComponent::setOnMuteChanged(std::function<void(bool)> cb) {
  onMuteChanged = std::move(cb);
}

void PanelComponent::setOnSoloChanged(std::function<void(bool)> cb) {
  onSoloChanged = std::move(cb);
}

void PanelComponent::setMuteState(bool muted) {
  muteButton.setToggleState(muted, juce::dontSendNotification);
}

void PanelComponent::setSoloState(bool soloed) {
  soloButton.setToggleState(soloed, juce::dontSendNotification);
}

void PanelComponent::setLevelProvider(std::function<float()> provider) {
  levelMeter.setLevelProvider(std::move(provider));
}
