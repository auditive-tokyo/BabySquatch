#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

/// Sub / Click / Direct 共通の Saturator（Drive + ClipType）
/// ヘッダオンリー。
namespace Saturator {

/// ClipType 定数
enum class ClipType { Soft = 0, Hard = 1, Tube = 2 };

/// Drive (dB) → 線形ゲイン変換
inline float driveGainFromDb(float db) noexcept {
  return std::pow(10.0f, db / 20.0f);
}

/// サンプルに Drive + Clip を適用して返す。
/// @param s        入力サンプル
/// @param driveDb  Drive 量 (0–24 dB)
/// @param clipType 0=Soft, 1=Hard, 2=Tube
inline float process(float s, float driveDb, int clipType) noexcept {
  s *= driveGainFromDb(driveDb);

  switch (clipType) {
  case 1: // Hard
    return juce::jlimit(-1.0f, 1.0f, s);
  case 2: { // Tube
    // 非対称ソフトクリップ → 偶数倍音（温かみ）
    constexpr float bias = 0.1f;
    return (s + bias) / (1.0f + std::abs(s + bias));
  }
  default: // Soft (tanh)
    return std::tanh(s);
  }
}

} // namespace Saturator
