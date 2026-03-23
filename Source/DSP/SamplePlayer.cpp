#include "SamplePlayer.h"
#include <algorithm>

// ────────────────────────────────────────────────────
// lifecycle
// ────────────────────────────────────────────────────

void SamplePlayer::prepare() {
  formatManager_.registerBasicFormats(); // WAV / AIFF
  playheadSamples_ = 0.0;
}

// ────────────────────────────────────────────────────
// サンプルロード（メッセージスレッドから）
// ────────────────────────────────────────────────────

void SamplePlayer::loadSample(const juce::File &file) {
  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager_.createReaderFor(file));
  if (reader == nullptr)
    return;

  // デコード（最大 30 秒）
  const auto maxSamples = static_cast<int>(
      std::min(reader->lengthInSamples,
               static_cast<juce::int64>(reader->sampleRate * 30.0)));
  juce::AudioBuffer<float> buf(static_cast<int>(reader->numChannels),
                               maxSamples);
  reader->read(&buf, 0, maxSamples, 0, true, true);

  // 常に 2ch（ステレオ）で保持。モノソースは ch0 を ch1 にコピー
  juce::AudioBuffer<float> stereo(2, maxSamples);
  stereo.copyFrom(0, 0, buf, 0, 0, maxSamples);
  if (buf.getNumChannels() >= 2)
    stereo.copyFrom(1, 0, buf, 1, 0, maxSamples);
  else
    stereo.copyFrom(1, 0, buf, 0, 0, maxSamples);

  const double fileSr = reader->sampleRate;

  // 波形サムネイル事前計算（モノ平均で表示）
  {
    constexpr int kThumbBins = 512;
    const int total = maxSamples;
    const float *srcL = stereo.getReadPointer(0);
    const float *srcR = stereo.getReadPointer(1);
    thumbMin_.resize(static_cast<std::size_t>(kThumbBins));
    thumbMax_.resize(static_cast<std::size_t>(kThumbBins));
    for (int bin = 0; bin < kThumbBins; ++bin) {
      const int s = (bin * total) / kThumbBins;
      const int e = ((bin + 1) * total) / kThumbBins;
      float mn = 0.0f;
      float mx = 0.0f;
      for (int j = s; j < e; ++j) {
        const float val = (srcL[j] + srcR[j]) * 0.5f;
        mn = std::min(mn, val);
        mx = std::max(mx, val);
      }
      thumbMin_[static_cast<std::size_t>(bin)] = mn;
      thumbMax_[static_cast<std::size_t>(bin)] = mx;
    }
    durationSec_.store(static_cast<double>(total) / fileSr);
  }

  // スピンロックで保護しながらスワップ
  {
    const juce::SpinLock::ScopedLockType lk(sampleLock_);
    buffer_ = std::move(stereo);
    sampleSampleRate_ = fileSr;
  }

  loaded_.store(true);
}

void SamplePlayer::unloadSample() {
  {
    const juce::SpinLock::ScopedLockType lk(sampleLock_);
    buffer_.setSize(0, 0);
  }
  thumbMin_.clear();
  thumbMax_.clear();
  durationSec_.store(0.0);
  playheadSamples_ = 0.0;
  loaded_.store(false);
}

// ────────────────────────────────────────────────────
// メタ情報
// ────────────────────────────────────────────────────

bool SamplePlayer::copyThumbnail(std::vector<float> &outMin,
                                 std::vector<float> &outMax) const noexcept {
  if (!loaded_.load())
    return false;
  outMin = thumbMin_;
  outMax = thumbMax_;
  return true;
}

// ────────────────────────────────────────────────────
// 読み出し
// ────────────────────────────────────────────────────

SamplePlayer::LockedView SamplePlayer::lock() noexcept {
  LockedView v;
  v.lock = std::make_unique<juce::SpinLock::ScopedTryLockType>(sampleLock_);
  if (v.lock->isLocked() && loaded_.load()) {
    v.data = buffer_.getReadPointer(0);
    v.dataR = buffer_.getNumChannels() >= 2 ? buffer_.getReadPointer(1)
                                            : buffer_.getReadPointer(0);
    v.length = buffer_.getNumSamples();
  }
  return v;
}

float SamplePlayer::readInterpolated(const float *srcData, int srcLen,
                                     double playRate, bool &finished) {
  if (playheadSamples_ >= static_cast<double>(srcLen - 1)) {
    finished = true;
    return 0.0f;
  }
  const auto i0 = static_cast<int>(playheadSamples_);
  const int i1 = juce::jmin(i0 + 1, srcLen - 1);
  const auto frac =
      static_cast<float>(playheadSamples_ - static_cast<double>(i0));
  const float s = srcData[i0] * (1.0f - frac) + srcData[i1] * frac;
  playheadSamples_ += playRate;
  return s;
}

std::pair<float, float> SamplePlayer::readInterpolatedStereo(const float *srcL,
                                                             const float *srcR,
                                                             int srcLen,
                                                             double playRate,
                                                             bool &finished) {
  if (playheadSamples_ >= static_cast<double>(srcLen - 1)) {
    finished = true;
    return {0.0f, 0.0f};
  }
  const auto i0 = static_cast<int>(playheadSamples_);
  const int i1 = juce::jmin(i0 + 1, srcLen - 1);
  const auto frac =
      static_cast<float>(playheadSamples_ - static_cast<double>(i0));
  const float l = srcL[i0] * (1.0f - frac) + srcL[i1] * frac;
  const float r = srcR[i0] * (1.0f - frac) + srcR[i1] * frac;
  playheadSamples_ += playRate;
  return {l, r};
}

std::pair<float, float> SamplePlayer::readNextStereo(double playRate,
                                                     bool &finished) {
  auto view = lock();
  if (!view) {
    finished = true;
    return {0.0f, 0.0f};
  }
  return readInterpolatedStereo(view.data, view.dataR, view.length, playRate,
                                finished);
}

float SamplePlayer::readNext(double playRate, bool &finished) {
  auto view = lock();
  if (!view) {
    finished = true;
    return 0.0f;
  }
  return readInterpolated(view.data, view.length, playRate, finished);
}
