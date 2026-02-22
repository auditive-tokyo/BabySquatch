#pragma once

#include <atomic>
#include <algorithm>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

/// Lock-free peak level detector.
/// Written by the audio thread (process()), read by the UI thread (getPeakDb()).
class LevelDetector {
public:
    /// process() をオーディオスレッドから毎ブロック呼び出す。
    /// data == nullptr のときは無音として扱い、ピークを減衰させる。
    void process(const float *data, int numSamples) noexcept {
        float blockPeak = 0.0f;
        if (data != nullptr) {
            for (int i = 0; i < numSamples; ++i)
                blockPeak = std::max(blockPeak, std::abs(data[i]));
        }
        // バリスティクス: ブロックピークを保持しつつ減衰
        currentPeak = std::max(blockPeak, currentPeak * decayPerBlock);
        peakLinear.store(currentPeak, std::memory_order_relaxed);
    }

    /// UIスレッドから呼び出して現在のピークレベル(dB)を取得。
    [[nodiscard]] float getPeakDb() const noexcept {
        const float peak = peakLinear.load(std::memory_order_relaxed);
        return juce::Decibels::gainToDecibels(peak, -100.0f);
    }

    /// prepareToPlay() から呼び出してリセット。
    void reset() noexcept {
        currentPeak = 0.0f;
        peakLinear.store(0.0f, std::memory_order_relaxed);
    }

    /// ブロック毎の減衰係数を設定（0に近いほど速い減衰）。
    /// デフォルト: 約1秒で-60dBに落ちる速さ（512サンプル/44.1kHz想定）。
    void setDecayPerBlock(float decay) noexcept { decayPerBlock = decay; }

private:
    std::atomic<float> peakLinear{0.0f}; ///< UIスレッドが読む
    float currentPeak{0.0f};             ///< オーディオスレッドのみ書き込み
    float decayPerBlock{0.97f};          ///< ブロック毎の乗算係数
};
