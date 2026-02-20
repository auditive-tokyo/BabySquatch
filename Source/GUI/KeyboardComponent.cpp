#include "KeyboardComponent.h"
#include "UIConstants.h"

KeyboardComponent::KeyboardComponent() {
  keyboard.setAvailableRange(12, 96); // C0–C7
  keyboard.setOctaveForMiddleC(4);
  keyboard.setKeyPressBaseOctave(keyPressOctave); // A → C2（低音域向け）
  addAndMakeVisible(keyboard);

  keyboardState.addListener(this);

  // モードボタン
  midiButton.onClick = [this] { setMode(Mode::midi); };
  fixedButton.onClick = [this] { setMode(Mode::fixed); };
  addAndMakeVisible(midiButton);
  addAndMakeVisible(fixedButton);
  updateButtonStates();
}

KeyboardComponent::~KeyboardComponent() { keyboardState.removeListener(this); }

void KeyboardComponent::resized() {
  auto area = getLocalBounds();

  // 上部にモードボタン
  auto buttonArea = area.removeFromTop(UIConstants::modeButtonHeight);
  const int buttonWidth = 60;
  midiButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2, 1));
  fixedButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2, 1));

  // 残りを鍵盤に
  keyboard.setBounds(area);
}

bool KeyboardComponent::keyPressed(const juce::KeyPress &key) {
  // Z: オクターブ下げ / X: オクターブ上げ
  if (key.getTextCharacter() == 'z' || key.getTextCharacter() == 'Z') {
    if (keyPressOctave > 0) {
      --keyPressOctave;
      keyboard.setKeyPressBaseOctave(keyPressOctave);
    }
    return true;
  }
  if (key.getTextCharacter() == 'x' || key.getTextCharacter() == 'X') {
    if (keyPressOctave < 7) {
      ++keyPressOctave;
      keyboard.setKeyPressBaseOctave(keyPressOctave);
    }
    return true;
  }
  return false;
}

void KeyboardComponent::grabFocus() { keyboard.grabKeyboardFocus(); }

void KeyboardComponent::setOnNoteFixed(std::function<void(int)> callback) {
  onNoteFixed = std::move(callback);
}

// ── MidiKeyboardState::Listener ──

void KeyboardComponent::handleNoteOn(juce::MidiKeyboardState *,
                                     int /*midiChannel*/, int midiNoteNumber,
                                     float /*velocity*/) {
  using enum Mode;
  if (currentMode != fixed || isReinjecting)
    return;

  // 同じノートをクリック → 固定解除
  if (fixedNote == midiNoteNumber) {
    const int oldNote = fixedNote;
    fixedNote = -1; // 先にクリアして再注入を防ぐ
    keyboardState.noteOff(1, oldNote, 0.0f);
    if (onNoteFixed)
      onNoteFixed(-1);
    return;
  }

  // 別のノート → 旧ノートをクリアして新ノートを固定（単音のみ）
  if (fixedNote >= 0) {
    const int oldNote = fixedNote;
    fixedNote = -1; // 先にクリアして再注入を防ぐ
    keyboardState.noteOff(1, oldNote, 0.0f);
  }

  fixedNote = midiNoteNumber;
  if (onNoteFixed)
    onNoteFixed(fixedNote);
}

void KeyboardComponent::handleNoteOff(juce::MidiKeyboardState *,
                                      int /*midiChannel*/, int midiNoteNumber,
                                      float /*velocity*/) {
  using enum Mode;
  // FIXEDモード: fixedNote が有効な場合のみ再注入
  if (currentMode == fixed && fixedNote >= 0 && midiNoteNumber == fixedNote) {
    juce::MessageManager::callAsync([this, note = fixedNote] {
      if (fixedNote != note)
        return; // 非同期中に変更されていたら中止
      isReinjecting = true;
      keyboardState.noteOn(1, note, 1.0f);
      isReinjecting = false;
    });
  }
}

// ── モード切り替え ──

void KeyboardComponent::setMode(Mode m) {
  using enum Mode;
  if (currentMode == m)
    return;

  // FIXEDモードを抜ける時に固定ノートをクリア
  if (currentMode == fixed && fixedNote >= 0) {
    keyboardState.noteOff(1, fixedNote, 0.0f);
    fixedNote = -1;
  }

  currentMode = m;
  updateButtonStates();

  keyboard.grabKeyboardFocus();
}

void KeyboardComponent::updateButtonStates() {
  using enum Mode;
  const auto activeColour = juce::Colour{0xFF00AAFF};
  const auto inactiveColour = juce::Colour{0xFF444444};

  midiButton.setColour(juce::TextButton::buttonColourId,
                       currentMode == midi ? activeColour : inactiveColour);
  fixedButton.setColour(juce::TextButton::buttonColourId,
                        currentMode == fixed ? activeColour : inactiveColour);

  midiButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  fixedButton.setColour(juce::TextButton::textColourOffId,
                        juce::Colours::white);
}
