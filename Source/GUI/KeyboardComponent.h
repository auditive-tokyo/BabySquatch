#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/// MIDI 鍵盤ラッパー（PCキーボードの Z/X オクターブシフト付き）
/// Standalone 専用機能を分離しておくことで、不要時に削除しやすい。
class KeyboardComponent : public juce::Component {
public:
  KeyboardComponent();
  ~KeyboardComponent() override = default;

  void resized() override;
  bool keyPressed(const juce::KeyPress &key) override;

  /// 内部の MidiKeyboardComponent にフォーカスを渡す
  void grabFocus();

  juce::MidiKeyboardState &getKeyboardState() { return keyboardState; }

private:
  juce::MidiKeyboardState keyboardState;
  juce::MidiKeyboardComponent keyboard{
      keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard};
  int keyPressOctave = 3; // PCキーのベースオクターブ（A → C2）
};
