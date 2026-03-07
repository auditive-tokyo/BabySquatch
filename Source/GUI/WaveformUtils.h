#pragma once

#include <juce_core/juce_core.h>
#include <utility>
#include <vector>

// ────────────────────────────────────────────────────────────────
// WaveformUtils
//   サムネイル波形プレビュー描画用ヘルパー。
//   ClickParams.cpp / DirectParams.cpp 共通で使用。
// ────────────────────────────────────────────────────────────────
namespace WaveformUtils {

/// 時刻 timeSec における波形サムネイルの振幅 (min, max) を
/// A → Hold → R エンベロープでスケールして返す。
/// 範囲外または durSec <= 0 の場合は {0, 0} を返す。
inline std::pair<float, float>
computePreview(const std::vector<float> &thumbMin,
               const std::vector<float> &thumbMax, double durSec,
               float attackSec, float holdSec, float releaseSec,
               float timeSec) {

  // A → Hold → R エンベロープ
  float env = 0.0f;
  if (timeSec < attackSec) {
    env = (attackSec > 0.0f) ? timeSec / attackSec : 1.0f;
  } else if (timeSec < attackSec + holdSec) {
    env = 1.0f;
  } else {
    const float rel = timeSec - attackSec - holdSec;
    env = (releaseSec > 0.0f)
              ? juce::jlimit(0.0f, 1.0f, 1.0f - rel / releaseSec)
              : 0.0f;
  }

  // サムネイル参照
  if (durSec <= 0.0 || timeSec < 0.0f || timeSec >= static_cast<float>(durSec))
    return {0.0f, 0.0f};

  const float t = timeSec / static_cast<float>(durSec);
  const auto n = static_cast<int>(thumbMin.size());
  const int idx =
      juce::jlimit(0, n - 1, static_cast<int>(t * static_cast<float>(n)));

  return {thumbMin[static_cast<std::size_t>(idx)] * env,
          thumbMax[static_cast<std::size_t>(idx)] * env};
}

} // namespace WaveformUtils
