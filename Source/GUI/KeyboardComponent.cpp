#include "KeyboardComponent.h"

KeyboardComponent::KeyboardComponent(juce::MidiKeyboardState &state)
    : keyboardState(state) {
  keyboard.setAvailableRange(12, 84); // C0–C6
  keyboard.setOctaveForMiddleC(4);
  keyboard.setKeyPressBaseOctave(keyPressOctave); // A → C2（低音域向け）
  keyboard.setWantsKeyboardFocus(false);          // KeyListenerで代替
  addAndMakeVisible(keyboard);
}

KeyboardComponent::~KeyboardComponent() {
  if (registeredTop_ != nullptr)
    registeredTop_->removeKeyListener(this);
}

void KeyboardComponent::resized() { keyboard.setBounds(getLocalBounds()); }

void KeyboardComponent::registerOnTopLevel() {
  auto *top = getTopLevelComponent();
  if (top == registeredTop_)
    return;

  if (registeredTop_ != nullptr)
    registeredTop_->removeKeyListener(this);

  if (top != nullptr)
    top->addKeyListener(this);

  registeredTop_ = top;
}

void KeyboardComponent::parentHierarchyChanged() { registerOnTopLevel(); }

// ── KeyListener overrides ──────────────────────────────

bool KeyboardComponent::keyPressed(const juce::KeyPress &key,
                                   juce::Component *) {
  if (handleOctaveShift(key))
    return true;
  return keyboard.keyPressed(key);
}

bool KeyboardComponent::keyStateChanged(bool isKeyDown, juce::Component *) {
  return keyboard.keyStateChanged(isKeyDown);
}

// ── Private helpers ────────────────────────────────────

bool KeyboardComponent::handleOctaveShift(const juce::KeyPress &key) {
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
