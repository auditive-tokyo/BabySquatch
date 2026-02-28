#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

/// Click チャンネルの DSP エンジン。
///   mode=1 (Tone)  : インパルス → BPF1(freq1/focus1) → BPF2(freq2/focus2)
///   mode=2 (Noise) : ホワイトノイズ → BPF 励起型（セルフレゾナント）
///   HPF/LPF は両モード共通のポスト EQ
class ClickEngine {
public:
  // ── lifecycle ──
  void prepareToPlay(double sampleRate, int samplesPerBlock);

  /// MIDI NoteOn 時のトリガー
  void triggerNote();

  /// 1 ブロック分をレンダリングし、buffer に加算する。
  void render(juce::AudioBuffer<float> &buffer, int numSamples, bool clickPass,
              double sampleRate);

  // ── UI→DSP setter（スレッドセーフ） ──
  void setMode(int m) { mode_.store(m); }
  void setGainDb(float db) { gainDb_.store(db); }
  void setDecayMs(float ms) { decayMs_.store(ms); }
  /// Tone:  Osc 開始周波数 + BPF1 中心 /  Noise: BPF1 中心周波数
  void setFreq1(float hz) { freq1_.store(hz); }
  /// Tone:  BPF1 Q（0=バイパス）       /  Noise: BPF1 Q（高いほどリング）
  void setFocus1(float q) { focus1_.store(q); }
  void setFreq2(float hz) { freq2_.store(hz); }
  void setFocus2(float q) { focus2_.store(q); }
  /// HPF カットオフ周波数 (Q=0 でバイパス)
  void setHpfFreq(float hz) { hpfFreq_.store(hz); }
  void setHpfQ(float q) { hpfQ_.store(q); }
  /// LPF カットオフ周波数 (Q=0 でバイパス)
  void setLpfFreq(float hz) { lpfFreq_.store(hz); }
  void setLpfQ(float q) { lpfQ_.store(q); }

  /// レベル計測用 scratchBuffer の先頭ポインタ
  const float *scratchData() const noexcept { return scratchBuffer_.data(); }

private:
  // ── render() の分割ヘルパー ──
  struct FilterFlags {
    bool bpf1;
    bool bpf2;
    bool hpf;
    bool lpf;
  };
  FilterFlags setupFilters(float sr);
  float synthesizeSample(int mode, const FilterFlags &flags);
  // ── 2 段 BPF ──
  juce::dsp::StateVariableTPTFilter<float> bpf1_; // freq1 / focus1
  juce::dsp::StateVariableTPTFilter<float> bpf2_; // freq2 / focus2
  // ── ポスト HPF / LPF ──
  juce::dsp::StateVariableTPTFilter<float> hpf_;
  juce::dsp::StateVariableTPTFilter<float> lpf_;

  juce::Random random_; // Noise モード用 RNG

  std::vector<float> scratchBuffer_;
  float noteTimeSamples_{0.0f};
  std::atomic<bool> active_{false};

  std::atomic<int> mode_{1}; // 1=Tone, 2=Noise, 3=Sample
  std::atomic<float> gainDb_{0.0f};
  std::atomic<float> decayMs_{50.0f};
  std::atomic<float> freq1_{5000.0f};   // BPF1 中心周波数
  std::atomic<float> focus1_{0.71f};    // BPF1 Q
  std::atomic<float> freq2_{10000.0f};  // BPF2 中心周波数
  std::atomic<float> focus2_{0.0f};     // BPF2 Q (0=bypass)
  std::atomic<float> hpfFreq_{200.0f};  // HPF カットオフ
  std::atomic<float> hpfQ_{0.0f};       // HPF Q (0=bypass)
  std::atomic<float> lpfFreq_{8000.0f}; // LPF カットオフ
  std::atomic<float> lpfQ_{0.0f};       // LPF Q (0=bypass)
};
