#pragma once

#include <vector>

/// Oomph用 Wavetable オシレーター（オーディオスレッドで使用）
/// 1周期分のサイン波をテーブルに事前生成し、再生時は線形補間で読み出す
/// triggerNote() で発音開始、setFrequencyHz() で毎サンプル周波数更新
class OomphOscillator {
public:
  OomphOscillator() = default;
  ~OomphOscillator() = default;

  /// processBlock 前に呼び出し（サンプルレート設定 + テーブル構築）
  void prepareToPlay(double sampleRate);

  /// 発音開始（トリガーのみ、ピッチは setFrequencyHz で制御）
  void triggerNote();

  /// 発音停止
  void stopNote();

  /// 周波数を設定（Hz、毎サンプル呼び出し可）
  void setFrequencyHz(float hz);

  /// 次のサンプルを生成（オーディオスレッドから呼び出し）
  float getNextSample();

  /// 発音中かどうか
  bool isActive() const { return active; }

private:
  static constexpr int tableSize = 2048;
  std::vector<float> wavetable; // tableSize + 1 要素（wrap用）

  bool active = false;
  double sampleRate = 44100.0;
  float currentIndex = 0.0f;
  float tableDelta = 0.0f; // phaseIncrementに相当

  void buildWavetable();
};
