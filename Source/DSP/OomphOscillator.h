#pragma once

/// Oomph用サイン波オシレーター（オーディオスレッドで使用）
/// MIDIノート → Hz 変換 → サイン波生成
class OomphOscillator {
public:
  OomphOscillator() = default;
  ~OomphOscillator() = default;

  /// processBlock 前に呼び出し（サンプルレート設定）
  void prepareToPlay(double sampleRate);

  /// MIDIノートを設定（-1 でサイレンス）
  void setNote(int midiNoteNumber);

  /// 現在のノート番号を取得（-1 = 無音）
  int getCurrentNote() const { return currentNote; }

  /// 次のサンプルを生成（オーディオスレッドから呼び出し）
  float getNextSample();

  /// 発音中かどうか
  bool isActive() const { return currentNote >= 0; }

private:
  int currentNote = -1;
  double sampleRate = 44100.0;
  double phase = 0.0;
  double phaseIncrement = 0.0;
};
