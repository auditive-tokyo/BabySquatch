#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/// MIDI 鍵盤ラッパー（トリガー専用）
/// - PCキーボード入力は常時有効（フォーカス不問）
/// - Z/Xでオクターブシフト
class KeyboardComponent : public juce::Component, private juce::KeyListener {
public:
  explicit KeyboardComponent(juce::MidiKeyboardState &state);
  ~KeyboardComponent() override;

  void resized() override;
  void parentHierarchyChanged() override;

  int getPreferredWidth() const {
    return static_cast<int>(keyboard.getTotalKeyboardWidth());
  }

  juce::MidiKeyboardState &getKeyboardState() { return keyboardState; }

  using juce::Component::keyPressed;      // unhide 1-arg overload
  using juce::Component::keyStateChanged; // unhide 1-arg overload

private:
  bool keyPressed(const juce::KeyPress &key, juce::Component *origin) override;
  bool keyStateChanged(bool isKeyDown, juce::Component *origin) override;

  void registerOnTopLevel();
  bool handleOctaveShift(const juce::KeyPress &key);

  int keyPressOctave = 3;
  juce::Component *registeredTop_ = nullptr;

  juce::MidiKeyboardState &keyboardState;
  juce::MidiKeyboardComponent keyboard{
      keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard};
};
