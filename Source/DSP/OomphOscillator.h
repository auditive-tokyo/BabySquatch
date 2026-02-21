#pragma once

#include <vector>

/// Oomph用 Wavetable オシレーター（オーディオスレッドで使用）
/// 1周期分のサイン波をテーブルに事前生成し、再生時は線形補間で読み出す
/// MIDIノート → Hz 変換 → テーブルインデックス進行
class OomphOscillator {
public:
  OomphOscillator() = default;
  ~OomphOscillator() = default;

  /// processBlock 前に呼び出し（サンプルレート設定 + テーブル構築）
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
  static constexpr int tableSize = 2048;
  std::vector<float> wavetable; // tableSize + 1 要素（wrap用）

  int currentNote = -1;
  double sampleRate = 44100.0;
  float currentIndex = 0.0f;
  float tableDelta = 0.0f; // phaseIncrementに相当

  void buildWavetable();
};
