#include "DirectEngine.h"
#include "Saturator.h"

#include <cmath>
#include <span>

// ────────────────────────────────────────────────────
// lifecycle
// ────────────────────────────────────────────────────

void DirectEngine::prepareToPlay(double sampleRate, int samplesPerBlock) {
  using enum juce::dsp::StateVariableTPTFilterType;
  juce::dsp::ProcessSpec spec{};
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
  spec.numChannels = 2;
  for (auto &f : hpfs_) {
    f.prepare(spec);
    f.setType(highpass);
  }
  for (auto &f : lpfs_) {
    f.prepare(spec);
    f.setType(lowpass);
  }

  scratchBuffer_.resize(static_cast<std::size_t>(samplesPerBlock));
  noteTimeSamples_ = 0.0f;
  startOffset_ = 0;
  active_.store(false);

  cachedSampleRate_ = static_cast<float>(sampleRate);
  ramp_.length = static_cast<int>(cachedSampleRate_ * 0.0005f);
  if (ramp_.length < 1)
    ramp_.length = 1;
  ramp_.prevAmp = 0.0f;
  ramp_.counter = 0;

  sampler_.prepare();
  directAmpLut_.reset();
}

void DirectEngine::triggerNote(int sampleOffset) {
  // パススルーモード時はサンプル不要でトリガー可能
  if (!passthroughMode_.load() && !sampler_.isLoaded())
    return;

  // サンプルモード時のみプレイヘッドリセット
  if (!passthroughMode_.load())
    sampler_.resetPlayhead();

  // リトリガー時エンベロープ不連続防止: 現在の amp を保存しランプ開始
  if (active_.load() && noteTimeSamples_ > 0.0f) {
    const float noteTimeMs = noteTimeSamples_ * 1000.0f / cachedSampleRate_;
    ramp_.prevAmp = computeSampleAmp(noteTimeMs);
    ramp_.counter = ramp_.length;
  } else {
    ramp_.prevAmp = 0.0f;
    ramp_.counter = 0;
  }

  noteTimeSamples_ = 0.0f;
  startOffset_ = sampleOffset;

  for (auto &f : hpfs_)
    f.reset();
  for (auto &f : lpfs_)
    f.reset();

  active_.store(true);
}

// ────────────────────────────────────────────────────
// プライベートヘルパー
// ────────────────────────────────────────────────────

auto DirectEngine::prepareFilters(float sr) -> FilterState {
  const float hpfF = juce::jlimit(20.0f, sr * 0.49f, hpfParams_.freq.load());
  const float hpfQv = hpfParams_.q.load();
  const int hpfStg = hpfParams_.stages.load();
  const float lpfF = juce::jlimit(20.0f, sr * 0.49f, lpfParams_.freq.load());
  const float lpfQv = lpfParams_.q.load();
  const int lpfStg = lpfParams_.stages.load();
  const bool doHpf = hpfF > 20.5f;
  const bool doLpf = lpfF < 19999.0f;

  if (doHpf) {
    const float qH = juce::jlimit(0.1f, 30.0f, hpfQv > 0.001f ? hpfQv : 0.707f);
    for (int i = 0; i < hpfStg; ++i) {
      hpfs_[static_cast<std::size_t>(i)].setCutoffFrequency(hpfF);
      hpfs_[static_cast<std::size_t>(i)].setResonance(qH);
    }
  }
  if (doLpf) {
    const float qL = juce::jlimit(0.1f, 30.0f, lpfQv > 0.001f ? lpfQv : 0.707f);
    for (int i = 0; i < lpfStg; ++i) {
      lpfs_[static_cast<std::size_t>(i)].setCutoffFrequency(lpfF);
      lpfs_[static_cast<std::size_t>(i)].setResonance(qL);
    }
  }
  return {doHpf, doLpf, hpfStg, lpfStg};
}

float DirectEngine::processFilterChain(const FilterState &fs, int ch, float s) {
  s = Saturator::process(s, driveDb_.load(), clipType_.load());
  for (int fi = 0; fs.doHpf && fi < fs.hpfStg; ++fi)
    s = hpfs_[static_cast<std::size_t>(fi)].processSample(ch, s);
  for (int fi = 0; fs.doLpf && fi < fs.lpfStg; ++fi)
    s = lpfs_[static_cast<std::size_t>(fi)].processSample(ch, s);
  if (fs.doHpf || fs.doLpf)
    s = Saturator::process(s, 0.0f, clipType_.load());
  return s;
}

float DirectEngine::computeMaxTimeSamples(float sr, double playRate) const {
  const float ampDurMs = maxDurationMs_.load();
  const float ampDurSamples = ampDurMs * sr / 1000.0f;
  const double dur = sampler_.durationSec();
  const float samplerDurSamples =
      (dur > 0.0 && playRate > 0.0)
          ? static_cast<float>(dur / playRate * static_cast<double>(sr))
          : 1e9f;
  return std::min(ampDurSamples, samplerDurSamples);
}

float DirectEngine::computeSampleAmp(float noteTimeMs) const {
  return EnvelopeLutManager::computeAmp(
      directAmpLut_.getActiveLut(), directAmpLut_.getDurationMs(), noteTimeMs);
}

// ────────────────────────────────────────────────────
// render
// ────────────────────────────────────────────────────

void DirectEngine::render(juce::AudioBuffer<float> &buffer, int numSamples,
                          bool directPass, double sampleRate) {
  if (!active_.load()) {
    std::fill_n(scratchBuffer_.data(), static_cast<std::size_t>(numSamples),
                0.0f);
    return;
  }

  const auto sr = static_cast<float>(sampleRate);
  const float gain = juce::Decibels::decibelsToGain(gainDb_.load());
  const double playRate =
      static_cast<double>(std::pow(2.0f, pitchSemitones_.load() / 12.0f)) *
      sampler_.sampleRate() / sampleRate;

  const float maxTimeSamples = computeMaxTimeSamples(sr, playRate);
  const FilterState fs = prepareFilters(sr);

  auto view = sampler_.lock();
  if (!view) {
    std::fill_n(scratchBuffer_.data(), static_cast<std::size_t>(numSamples),
                0.0f);
    return;
  }
  // readInterpolatedStereo() からアクセスするためメンバーにキャッシュ
  viewCache_.data = view.data;
  viewCache_.dataR = view.dataR ? view.dataR : view.data;
  viewCache_.length = view.length;

  const int numCh = buffer.getNumChannels();

  for (int i = 0; i < numSamples; ++i) {
    if (startOffset_ > 0) {
      --startOffset_;
      scratchBuffer_[static_cast<std::size_t>(i)] = 0.0f;
      continue;
    }

    if (noteTimeSamples_ >= maxTimeSamples) {
      active_.store(false);
      std::fill_n(scratchBuffer_.data() + i,
                  static_cast<std::size_t>(numSamples - i), 0.0f);
      break;
    }

    const float noteTimeMs = noteTimeSamples_ * 1000.0f / sr;
    const float amp = computeSampleAmp(noteTimeMs);

    bool finished = false;
    auto [sL, sR] = sampler_.readInterpolatedStereo(
        viewCache_.data, viewCache_.dataR, viewCache_.length, playRate, finished);
    if (finished) {
      sL = 0.0f;
      sR = 0.0f;
    }
    sL = processFilterChain(fs, 0, sL) * amp * gain;
    sR = processFilterChain(fs, 1, sR) * amp * gain;

    scratchBuffer_[static_cast<std::size_t>(i)] = (sL + sR) * 0.5f;
    if (directPass) {
      buffer.addSample(0, i, sL);
      if (numCh >= 2)
        buffer.addSample(1, i, sR);
    }
    noteTimeSamples_ += 1.0f;
  }

  viewCache_.clear();
}

// ────────────────────────────────────────────────
// renderPassthrough
// ────────────────────────────────────────────────

float DirectEngine::computePassthroughAmp(float sr, float maxTimeSamples) {
  if (!active_.load())
    return 0.0f;

  if (startOffset_ > 0) {
    --startOffset_;
    return ramp_.prevAmp; // リトリガー時はゼロ落ちせず旧アンプを維持
  }

  if (noteTimeSamples_ >= maxTimeSamples) {
    active_.store(false);
    return 0.0f;
  }

  const float noteTimeMs = noteTimeSamples_ * 1000.0f / sr;
  float amp = computeSampleAmp(noteTimeMs);

  // リトリガーランプ: 旧アンプ → 新アンプへスムーズ遷移
  if (ramp_.counter > 0) {
    const float t =
        static_cast<float>(ramp_.counter) / static_cast<float>(ramp_.length);
    amp = ramp_.prevAmp * t + amp * (1.0f - t);
    --ramp_.counter;
  }

  noteTimeSamples_ += 1.0f;
  return amp;
}

void DirectEngine::renderPassthrough(juce::AudioBuffer<float> &buffer,
                                     std::span<const float> inputL,
                                     std::span<const float> inputR,
                                     int numSamples, double sampleRate) {
  const auto sr = static_cast<float>(sampleRate);
  const float gain = juce::Decibels::decibelsToGain(gainDb_.load());

  // 停止判定用最大再生時間（パススルーではサンプル長がないので期間のみ）
  const float ampDurMs = maxDurationMs_.load();
  const float maxTimeSamples = ampDurMs * sr / 1000.0f;

  const FilterState fs = prepareFilters(sr);
  const int numCh = buffer.getNumChannels();

  for (int i = 0; i < numSamples; ++i) {
    const float amp = computePassthroughAmp(sr, maxTimeSamples);

    const auto idx = static_cast<std::size_t>(i);
    float sL = (i < static_cast<int>(inputL.size())) ? inputL[idx] : 0.0f;
    float sR = (i < static_cast<int>(inputR.size())) ? inputR[idx] : 0.0f;

    // amp または入力がゼロなら出力しない（Tube バイアス漏れ防止も兼ねる）
    if (amp == 0.0f || (sL == 0.0f && sR == 0.0f)) {
      scratchBuffer_[idx] = 0.0f;
      continue;
    }

    sL = processFilterChain(fs, 0, sL);
    sR = processFilterChain(fs, 1, sR);

    sL *= gain * amp;
    sR *= gain * amp;

    scratchBuffer_[idx] = (sL + sR) * 0.5f;
    buffer.addSample(0, i, sL);
    if (numCh >= 2)
      buffer.addSample(1, i, sR);
  }
}
