#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/// MIDI 鍵盤ラッパー（トリガー専用）
/// - PCキーボード入力可、Z/Xオクターブシフト
class KeyboardComponent : public juce::Component {
public:
  explicit KeyboardComponent(juce::MidiKeyboardState &state);

  void resized() override;
  bool keyPressed(const juce::KeyPress &key) override;

  /// 内部の MidiKeyboardComponent にフォーカスを渡す
  void grabFocus();

  juce::MidiKeyboardState &getKeyboardState() { return keyboardState; }

private:
  int keyPressOctave = 3;

  juce::MidiKeyboardState &keyboardState;
  juce::MidiKeyboardComponent keyboard{
      keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard};
};
