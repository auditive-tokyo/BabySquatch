#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

BabySquatchAudioProcessor::BabySquatchAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

BabySquatchAudioProcessor::~BabySquatchAudioProcessor() = default;

const juce::String // NOSONAR: JUCE API
BabySquatchAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool BabySquatchAudioProcessor::acceptsMidi() const { return true; }

bool BabySquatchAudioProcessor::producesMidi() const { return false; }

bool BabySquatchAudioProcessor::isMidiEffect() const { return false; }

double BabySquatchAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int BabySquatchAudioProcessor::getNumPrograms() { return 1; }

int BabySquatchAudioProcessor::getCurrentProgram() { return 0; }

void BabySquatchAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String // NOSONAR: JUCE API
BabySquatchAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void BabySquatchAudioProcessor::changeProgramName(int index,
                                                  const juce::String &newName) {
  juce::ignoreUnused(index, newName);
}

void BabySquatchAudioProcessor::prepareToPlay(double sampleRate,
                                              int samplesPerBlock) {
  oomphOsc.prepareToPlay(sampleRate);
  oomphScratchBuffer.resize(static_cast<size_t>(samplesPerBlock));

  // LUT の初期値: 全スロット 1.0（ユニティゲイン）
  envLut0.fill(1.0f);
  envLut1.fill(1.0f);
  envLutActiveIndex.store(0);
  noteTimeSamples = 0.0f;
}

void BabySquatchAudioProcessor::releaseResources() {
  // Currently no resources to release - will be populated when adding DSP
  // processing
}

bool BabySquatchAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
    return false;

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;

  return true;
}

void BabySquatchAudioProcessor::pushWaveformBlock(const float *data,
                                                  int numSamples) noexcept {
  int start1 = 0;
  int size1 = 0;
  int start2 = 0;
  int size2 = 0;
  waveformFifo.prepareToWrite(numSamples, start1, size1, start2, size2);

  if (size1 > 0)
    std::copy_n(data, size1, waveformBuffer.data() + start1);
  if (size2 > 0)
    std::copy_n(data + size1, size2, waveformBuffer.data() + start2);

  waveformFifo.finishedWrite(size1 + size2);
}

int BabySquatchAudioProcessor::popWaveformSamples(float *destination,
                                                  int maxSamples) noexcept {
  if (destination == nullptr || maxSamples <= 0)
    return 0;

  int start1 = 0;
  int size1 = 0;
  int start2 = 0;
  int size2 = 0;
  waveformFifo.prepareToRead(maxSamples, start1, size1, start2, size2);

  if (size1 > 0)
    std::copy_n(waveformBuffer.data() + start1, size1, destination);
  if (size2 > 0)
    std::copy_n(waveformBuffer.data() + start2, size2, destination + size1);

  waveformFifo.finishedRead(size1 + size2);
  return size1 + size2;
}

void BabySquatchAudioProcessor::bakeEnvelopeLut(const float *data, int size) {
  // 非アクティブ側バッファに書き込み → アトミックフリップ
  const int activeIdx = envLutActiveIndex.load(std::memory_order_acquire);
  auto &dst = (activeIdx == 0) ? envLut1 : envLut0;

  // ソースを LUT サイズにリサンプル（線形補間）
  for (int i = 0; i < envLutSize; ++i) {
    const float srcPos = static_cast<float>(i) /
                         static_cast<float>(envLutSize - 1) *
                         static_cast<float>(size - 1);
    const auto idx0 = static_cast<int>(srcPos);
    const auto idx1 = std::min(idx0 + 1, size - 1);
    const float frac = srcPos - static_cast<float>(idx0);
    dst[static_cast<size_t>(i)] =
        data[idx0] * (1.0f - frac) +
        data[idx1] * frac; // NOLINT: pointer arithmetic
  }

  // フリップ: オーディオスレッドが次回から新しい側を読む
  envLutActiveIndex.store(1 - activeIdx, std::memory_order_release);
}

void BabySquatchAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  // FIXEDモード時はGUI鍵盤のMIDIマージとOSC発音をスキップ
  if (fixedModeActive.load()) {
    // FIXEDモード移行時に発音中のOSCを停止
    if (oomphOsc.isActive())
      oomphOsc.setNote(-1);
  } else {
    // GUI鍵盤のMIDIイベントをバッファにマージ
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(),
                                        true);

    // MIDIバッファを走査 → OomphOscillator に通知
    for (const auto metadata : midiMessages) {
      const auto msg = metadata.getMessage();
      if (msg.isNoteOn()) {
        oomphOsc.setNote(msg.getNoteNumber());
        noteTimeSamples = 0.0f; // エンベロープリセット
      } else if (msg.isNoteOff() &&
                 msg.getNoteNumber() == oomphOsc.getCurrentNote()) {
        oomphOsc.setNote(-1);
      }
    }
  }

  // サイン波を出力バッファに加算（LUT エンベロープ適用）
  const int numSamples = buffer.getNumSamples();
  if (oomphOsc.isActive()) {
    const float gain = juce::Decibels::decibelsToGain(oomphGainDb.load());
    const int numChannels = buffer.getNumChannels();
    const int lutIdx = envLutActiveIndex.load(std::memory_order_acquire);
    const auto &lut = (lutIdx == 0) ? envLut0 : envLut1;
    const float durMs = envDurationMs.load();
    const auto sr = static_cast<float>(getSampleRate());

    for (int sample = 0; sample < numSamples; ++sample) {
      // LUT 参照でエンベロープゲインを取得
      const float noteTimeMs = noteTimeSamples * 1000.0f / sr;
      const float lutPos =
          (durMs > 0.0f)
              ? (noteTimeMs / durMs) * static_cast<float>(envLutSize - 1)
              : 0.0f;
      const auto lutIndex = std::min(static_cast<int>(lutPos), envLutSize - 1);
      const float envGain = lut[static_cast<size_t>(lutIndex)];

      const float oscSample = oomphOsc.getNextSample() * gain * envGain;
      oomphScratchBuffer[static_cast<size_t>(sample)] = oscSample;
      for (int ch = 0; ch < numChannels; ++ch)
        buffer.addSample(ch, sample, oscSample);

      noteTimeSamples += 1.0f;
    }
  } else {
    std::fill_n(oomphScratchBuffer.data(), numSamples, 0.0f);
  }
}

bool BabySquatchAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *BabySquatchAudioProcessor::createEditor() {
  // clang-format off
  return new BabySquatchAudioProcessorEditor(*this); // NOSONAR: DAW host takes ownership
  // clang-format on
}

void BabySquatchAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  juce::ignoreUnused(destData);
}

void BabySquatchAudioProcessor::setStateInformation(
    const void *data, // NOSONAR: JUCE API
    int sizeInBytes) {
  juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new BabySquatchAudioProcessor(); // NOSONAR: DAW host takes ownership
}
