#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/// MIDI 鍵盤ラッパー（MIDI/FIXED モード切り替え付き）
/// - MIDI モード: PCキーボード入力可、Z/Xオクターブシフト
/// - FIXED モード: クリックしたノートを固定記憶
class KeyboardComponent : public juce::Component,
                          private juce::MidiKeyboardState::Listener {
public:
  enum class Mode { midi, fixed };

  explicit KeyboardComponent(juce::MidiKeyboardState &state);
  ~KeyboardComponent() override;

  void resized() override;
  bool keyPressed(const juce::KeyPress &key) override;

  /// 内部の MidiKeyboardComponent にフォーカスを渡す
  void grabFocus();

  Mode getMode() const { return currentMode; }
  int getFixedNote() const { return fixedNote; }

  juce::MidiKeyboardState &getKeyboardState() { return keyboardState; }

  /// FIXEDモードでノートが固定されたときのコールバック (int noteNumber)
  void setOnNoteFixed(std::function<void(int)> callback);

private:
  void handleNoteOn(juce::MidiKeyboardState *, int midiChannel,
                    int midiNoteNumber, float velocity) override;
  void handleNoteOff(juce::MidiKeyboardState *, int midiChannel,
                     int midiNoteNumber, float velocity) override;
  void setMode(Mode m);
  void updateButtonStates();

  Mode currentMode = Mode::midi;
  int fixedNote = -1; // 未選択
  int keyPressOctave = 3;
  bool isReinjecting = false; // noteOn 再注入のガード

  juce::MidiKeyboardState &keyboardState;
  juce::MidiKeyboardComponent keyboard{
      keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard};

  juce::TextButton midiButton{"MIDI"};
  juce::TextButton fixedButton{"FIXED"};

  std::function<void(int)> onNoteFixed;
};
