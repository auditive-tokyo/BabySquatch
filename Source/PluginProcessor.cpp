#include "PluginProcessor.h"
#include "DSP/EnvelopeData.h"
#include "GUI/LutBaker.h"
#include "ParamIDs.h"
#include "PluginEditor.h"
#include <span>

// ─────────────────────────────────────────────────────────────────────
// APVTS ParameterLayout
// ─────────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
BoomBabyAudioProcessor::createParameterLayout() {
  using FloatParam = juce::AudioParameterFloat;
  using ChoiceParam = juce::AudioParameterChoice;
  using BoolParam = juce::AudioParameterBool;
  using NRange = juce::NormalisableRange<float>;

  // ログスケールのレンジ生成ヘルパー
  auto logRange = [](float min, float max, float mid) {
    auto r = NRange(min, max, 0.01f);
    r.setSkewForCentre(mid);
    return r;
  };
  auto logRangeInt = [](float min, float max, float mid) {
    auto r = NRange(min, max, 1.0f);
    r.setSkewForCentre(mid);
    return r;
  };

  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // ===================== Sub =====================
  layout.add(
      std::make_unique<ChoiceParam>(ParamIDs::subWaveShape, "Sub Wave",
                                    juce::StringArray{"Tri", "SQR", "SAW"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subLength, "Sub Length",
                                          logRangeInt(10.0f, 2000.0f, 300.0f),
                                          300.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subAmp, "Sub Amp",
                                          NRange(0.0f, 200.0f, 0.1f), 100.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subFreq, "Sub Freq",
                                          logRange(20.0f, 20000.0f, 200.0f),
                                          200.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subMix, "Sub Mix",
                                          NRange(-100.0f, 100.0f, 1.0f), 0.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subSatDrive, "Sub Drive",
                                          NRange(0.0f, 24.0f, 0.1f), 0.0f));
  layout.add(std::make_unique<ChoiceParam>(
      ParamIDs::subSatClipType, "Sub Clip",
      juce::StringArray{"Soft", "Hard", "Tube"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subTone1, "Sub Tone1",
                                          NRange(0.0f, 100.0f, 0.1f), 25.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subTone2, "Sub Tone2",
                                          NRange(0.0f, 100.0f, 0.1f), 25.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subTone3, "Sub Tone3",
                                          NRange(0.0f, 100.0f, 0.1f), 25.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subTone4, "Sub Tone4",
                                          NRange(0.0f, 100.0f, 0.1f), 25.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::subGain, "Sub Gain",
                                          NRange(-60.0f, 12.0f, 0.01f), 0.0f));
  layout.add(std::make_unique<BoolParam>(ParamIDs::subMute, "Sub Mute", false));
  layout.add(std::make_unique<BoolParam>(ParamIDs::subSolo, "Sub Solo", false));

  // ===================== Click =====================
  layout.add(std::make_unique<ChoiceParam>(ParamIDs::clickMode, "Click Mode",
                                           juce::StringArray{"Noise", "Sample"},
                                           0));
  layout.add(
      std::make_unique<FloatParam>(ParamIDs::clickNoiseDecay, "Click Decay",
                                   logRangeInt(1.0f, 2000.0f, 200.0f), 30.0f));
  layout.add(std::make_unique<FloatParam>(
      ParamIDs::clickBpf1Freq, "Click BPF Freq",
      logRangeInt(20.0f, 20000.0f, 1000.0f), 5000.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::clickBpf1Q, "Click BPF Q",
                                          NRange(0.1f, 18.0f, 0.01f), 0.71f));
  layout.add(
      std::make_unique<ChoiceParam>(ParamIDs::clickBpf1Slope, "Click BPF Slope",
                                    juce::StringArray{"12", "24", "48"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::clickSamplePitch,
                                          "Click Pitch",
                                          NRange(-24.0f, 24.0f, 1.0f), 0.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::clickSampleAmp, "Click Amp",
                                          NRange(0.0f, 200.0f, 0.1f), 100.0f));
  layout.add(std::make_unique<FloatParam>(
      ParamIDs::clickSampleDecay, "Click Sample Decay",
      logRangeInt(10.0f, 2000.0f, 300.0f), 300.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::clickDrive, "Click Drive",
                                          NRange(0.0f, 24.0f, 0.1f), 0.0f));
  layout.add(std::make_unique<ChoiceParam>(
      ParamIDs::clickClipType, "Click Clip",
      juce::StringArray{"Soft", "Hard", "Tube"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::clickHpfFreq, "Click HPF",
                                          logRangeInt(20.0f, 20000.0f, 1000.0f),
                                          20.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::clickHpfQ, "Click HPF Q",
                                          NRange(0.1f, 18.0f, 0.01f), 0.71f));
  layout.add(
      std::make_unique<ChoiceParam>(ParamIDs::clickHpfSlope, "Click HPF Slope",
                                    juce::StringArray{"12", "24", "48"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::clickLpfFreq, "Click LPF",
                                          logRangeInt(20.0f, 20000.0f, 1000.0f),
                                          20000.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::clickLpfQ, "Click LPF Q",
                                          NRange(0.1f, 18.0f, 0.01f), 0.71f));
  layout.add(
      std::make_unique<ChoiceParam>(ParamIDs::clickLpfSlope, "Click LPF Slope",
                                    juce::StringArray{"12", "24", "48"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::clickGain, "Click Gain",
                                          NRange(-60.0f, 12.0f, 0.01f), 0.0f));
  layout.add(
      std::make_unique<BoolParam>(ParamIDs::clickMute, "Click Mute", false));
  layout.add(
      std::make_unique<BoolParam>(ParamIDs::clickSolo, "Click Solo", false));

  // ===================== Direct =====================
  layout.add(
      std::make_unique<ChoiceParam>(ParamIDs::directMode, "Direct Mode",
                                    juce::StringArray{"Direct", "Sample"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directPitch, "Direct Pitch",
                                          NRange(-24.0f, 24.0f, 1.0f), 0.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directAmp, "Direct Amp",
                                          NRange(0.0f, 200.0f, 0.1f), 100.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directDrive, "Direct Drive",
                                          NRange(0.0f, 24.0f, 0.1f), 0.0f));
  layout.add(std::make_unique<ChoiceParam>(
      ParamIDs::directClipType, "Direct Clip",
      juce::StringArray{"Soft", "Hard", "Tube"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directDecay, "Direct Decay",
                                          logRangeInt(10.0f, 2000.0f, 300.0f),
                                          300.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directHpfFreq, "Direct HPF",
                                          logRangeInt(20.0f, 20000.0f, 1000.0f),
                                          20.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directHpfQ, "Direct HPF Q",
                                          NRange(0.1f, 18.0f, 0.01f), 0.707f));
  layout.add(std::make_unique<ChoiceParam>(
      ParamIDs::directHpfSlope, "Direct HPF Slope",
      juce::StringArray{"12", "24", "48"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directLpfFreq, "Direct LPF",
                                          logRangeInt(20.0f, 20000.0f, 1000.0f),
                                          20000.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directLpfQ, "Direct LPF Q",
                                          NRange(0.1f, 18.0f, 0.01f), 0.707f));
  layout.add(std::make_unique<ChoiceParam>(
      ParamIDs::directLpfSlope, "Direct LPF Slope",
      juce::StringArray{"12", "24", "48"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directThreshold,
                                          "Direct Thresh",
                                          NRange(-60.0f, 0.0f, 0.1f), -24.0f));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directHold, "Direct Hold",
                                          NRange(20.0f, 500.0f, 1.0f), 50.0f));
  layout.add(std::make_unique<ChoiceParam>(
      ParamIDs::directLookAhead, "Direct Look-ahead",
      juce::StringArray{"0 ms", "1.5 ms", "3 ms", "6 ms"}, 0));
  layout.add(std::make_unique<FloatParam>(ParamIDs::directGain, "Direct Gain",
                                          NRange(-60.0f, 12.0f, 0.01f), 0.0f));
  layout.add(
      std::make_unique<BoolParam>(ParamIDs::directMute, "Direct Mute", false));
  layout.add(
      std::make_unique<BoolParam>(ParamIDs::directSolo, "Direct Solo", false));

  // ===================== Master =====================
  layout.add(std::make_unique<FloatParam>(ParamIDs::masterGain, "Master Gain",
                                          NRange(-60.0f, 12.0f, 0.01f), 0.0f));

  return layout;
}

BoomBabyAudioProcessor::BoomBabyAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "BoomBabyState", createParameterLayout()) {
  registerParameterListeners();
}

BoomBabyAudioProcessor::~BoomBabyAudioProcessor() {
  // Listener を安全に解除（デストラクタ順序問題を防止）
  apvts_.removeParameterListener(ParamIDs::subWaveShape, this);
  apvts_.removeParameterListener(ParamIDs::subLength, this);
  apvts_.removeParameterListener(ParamIDs::subSatClipType, this);
  apvts_.removeParameterListener(ParamIDs::subTone1, this);
  apvts_.removeParameterListener(ParamIDs::subTone2, this);
  apvts_.removeParameterListener(ParamIDs::subTone3, this);
  apvts_.removeParameterListener(ParamIDs::subTone4, this);
  apvts_.removeParameterListener(ParamIDs::subGain, this);
  apvts_.removeParameterListener(ParamIDs::subMute, this);
  apvts_.removeParameterListener(ParamIDs::subSolo, this);
  apvts_.removeParameterListener(ParamIDs::clickMode, this);
  apvts_.removeParameterListener(ParamIDs::clickNoiseDecay, this);
  apvts_.removeParameterListener(ParamIDs::clickBpf1Freq, this);
  apvts_.removeParameterListener(ParamIDs::clickBpf1Q, this);
  apvts_.removeParameterListener(ParamIDs::clickBpf1Slope, this);
  apvts_.removeParameterListener(ParamIDs::clickSamplePitch, this);
  apvts_.removeParameterListener(ParamIDs::clickDrive, this);
  apvts_.removeParameterListener(ParamIDs::clickClipType, this);
  apvts_.removeParameterListener(ParamIDs::clickHpfFreq, this);
  apvts_.removeParameterListener(ParamIDs::clickHpfQ, this);
  apvts_.removeParameterListener(ParamIDs::clickHpfSlope, this);
  apvts_.removeParameterListener(ParamIDs::clickLpfFreq, this);
  apvts_.removeParameterListener(ParamIDs::clickLpfQ, this);
  apvts_.removeParameterListener(ParamIDs::clickLpfSlope, this);
  apvts_.removeParameterListener(ParamIDs::clickGain, this);
  apvts_.removeParameterListener(ParamIDs::clickMute, this);
  apvts_.removeParameterListener(ParamIDs::clickSolo, this);
  apvts_.removeParameterListener(ParamIDs::directMode, this);
  apvts_.removeParameterListener(ParamIDs::directPitch, this);
  apvts_.removeParameterListener(ParamIDs::directDrive, this);
  apvts_.removeParameterListener(ParamIDs::directClipType, this);
  apvts_.removeParameterListener(ParamIDs::directHpfFreq, this);
  apvts_.removeParameterListener(ParamIDs::directHpfQ, this);
  apvts_.removeParameterListener(ParamIDs::directHpfSlope, this);
  apvts_.removeParameterListener(ParamIDs::directLpfFreq, this);
  apvts_.removeParameterListener(ParamIDs::directLpfQ, this);
  apvts_.removeParameterListener(ParamIDs::directLpfSlope, this);
  apvts_.removeParameterListener(ParamIDs::directThreshold, this);
  apvts_.removeParameterListener(ParamIDs::directHold, this);
  apvts_.removeParameterListener(ParamIDs::directLookAhead, this);
  apvts_.removeParameterListener(ParamIDs::directGain, this);
  apvts_.removeParameterListener(ParamIDs::directMute, this);
  apvts_.removeParameterListener(ParamIDs::directSolo, this);
  apvts_.removeParameterListener(ParamIDs::masterGain, this);
}

// ─────────────────────────────────────────────────────────────────────────
// APVTS Listener 登録 / パラメータ→DSP 同期
// ─────────────────────────────────────────────────────────────────────────
void BoomBabyAudioProcessor::registerParameterListeners() {
  for (const auto *id : {ParamIDs::subWaveShape,   ParamIDs::subLength,
                         ParamIDs::subSatClipType, ParamIDs::subTone1,
                         ParamIDs::subTone2,       ParamIDs::subTone3,
                         ParamIDs::subTone4,       ParamIDs::subGain,
                         ParamIDs::subMute,        ParamIDs::subSolo,
                         ParamIDs::clickMode,      ParamIDs::clickNoiseDecay,
                         ParamIDs::clickBpf1Freq,  ParamIDs::clickBpf1Q,
                         ParamIDs::clickBpf1Slope, ParamIDs::clickSamplePitch,
                         ParamIDs::clickDrive,     ParamIDs::clickClipType,
                         ParamIDs::clickHpfFreq,   ParamIDs::clickHpfQ,
                         ParamIDs::clickHpfSlope,  ParamIDs::clickLpfFreq,
                         ParamIDs::clickLpfQ,      ParamIDs::clickLpfSlope,
                         ParamIDs::clickGain,      ParamIDs::clickMute,
                         ParamIDs::clickSolo,      ParamIDs::directMode,
                         ParamIDs::directPitch,    ParamIDs::directDrive,
                         ParamIDs::directClipType, ParamIDs::directHpfFreq,
                         ParamIDs::directHpfQ,     ParamIDs::directHpfSlope,
                         ParamIDs::directLpfFreq,  ParamIDs::directLpfQ,
                         ParamIDs::directLpfSlope, ParamIDs::directThreshold,
                         ParamIDs::directHold,     ParamIDs::directLookAhead,
                         ParamIDs::directGain,     ParamIDs::directMute,
                         ParamIDs::directSolo,     ParamIDs::masterGain})
    apvts_.addParameterListener(id, this);
}

namespace {
constexpr std::array kSlopes = {12, 24, 48};
constexpr std::array<float, 4> kLookAheadMs = {0.0f, 1.5f, 3.0f, 6.0f};
} // namespace

void BoomBabyAudioProcessor::parameterChanged(const juce::String &parameterID,
                                              float newValue) {
  const auto v = newValue;
  const auto idx = static_cast<int>(v);

  // ── Sub ──
  if (parameterID == ParamIDs::subWaveShape) {
    subEngine_.oscillator().setWaveShape(static_cast<WaveShape>(idx + 1));
  } else if (parameterID == ParamIDs::subLength) {
    subEngine_.setLengthMs(v);
  } else if (parameterID == ParamIDs::subSatClipType) {
    subEngine_.oscillator().setClipType(idx);
  } else if (parameterID == ParamIDs::subTone1) {
    subEngine_.oscillator().setHarmonicGain(1, v / 100.0f);
  } else if (parameterID == ParamIDs::subTone2) {
    subEngine_.oscillator().setHarmonicGain(2, v / 100.0f);
  } else if (parameterID == ParamIDs::subTone3) {
    subEngine_.oscillator().setHarmonicGain(3, v / 100.0f);
  } else if (parameterID == ParamIDs::subTone4) {
    subEngine_.oscillator().setHarmonicGain(4, v / 100.0f);
  } else if (parameterID == ParamIDs::subGain) {
    subEngine_.setGainDb(v);
  } else if (parameterID == ParamIDs::subMute) {
    channelState_.setMute(ChannelState::Channel::sub, v >= 0.5f);
  } else if (parameterID == ParamIDs::subSolo) {
    channelState_.setSolo(ChannelState::Channel::sub, v >= 0.5f);
  }
  // ── Click ──
  else if (parameterID == ParamIDs::clickMode) {
    clickEngine_.setMode(idx + 1); // APVTS 0=Noise→1, 1=Sample→2
  } else if (parameterID == ParamIDs::clickNoiseDecay) {
    clickEngine_.setDecayMs(v);
  } else if (parameterID == ParamIDs::clickBpf1Freq) {
    clickEngine_.setFreq1(v);
  } else if (parameterID == ParamIDs::clickBpf1Q) {
    clickEngine_.setFocus1(v);
  } else if (parameterID == ParamIDs::clickBpf1Slope) {
    clickEngine_.setBpf1Slope(kSlopes[static_cast<std::size_t>(idx)]);
  } else if (parameterID == ParamIDs::clickSamplePitch) {
    clickEngine_.setPitchSemitones(v);
  } else if (parameterID == ParamIDs::clickDrive) {
    clickEngine_.setDriveDb(v);
  } else if (parameterID == ParamIDs::clickClipType) {
    clickEngine_.setClipType(idx);
  } else if (parameterID == ParamIDs::clickHpfFreq) {
    clickEngine_.setHpfFreq(v);
  } else if (parameterID == ParamIDs::clickHpfQ) {
    clickEngine_.setHpfQ(v);
  } else if (parameterID == ParamIDs::clickHpfSlope) {
    clickEngine_.setHpfSlope(kSlopes[static_cast<std::size_t>(idx)]);
  } else if (parameterID == ParamIDs::clickLpfFreq) {
    clickEngine_.setLpfFreq(v);
  } else if (parameterID == ParamIDs::clickLpfQ) {
    clickEngine_.setLpfQ(v);
  } else if (parameterID == ParamIDs::clickLpfSlope) {
    clickEngine_.setLpfSlope(kSlopes[static_cast<std::size_t>(idx)]);
  } else if (parameterID == ParamIDs::clickGain) {
    clickEngine_.setGainDb(v);
  } else if (parameterID == ParamIDs::clickMute) {
    channelState_.setMute(ChannelState::Channel::click, v >= 0.5f);
  } else if (parameterID == ParamIDs::clickSolo) {
    channelState_.setSolo(ChannelState::Channel::click, v >= 0.5f);
  }
  // ── Direct ──
  else if (parameterID == ParamIDs::directMode) {
    setDirectSampleMode(idx == 1); // 0=Direct(passthrough), 1=Sample
  } else if (parameterID == ParamIDs::directPitch) {
    directEngine_.setPitchSemitones(v);
  } else if (parameterID == ParamIDs::directDrive) {
    directEngine_.setDriveDb(v);
  } else if (parameterID == ParamIDs::directClipType) {
    directEngine_.setClipType(idx);
  } else if (parameterID == ParamIDs::directHpfFreq) {
    directEngine_.setHpfFreq(v);
  } else if (parameterID == ParamIDs::directHpfQ) {
    directEngine_.setHpfQ(v);
  } else if (parameterID == ParamIDs::directHpfSlope) {
    directEngine_.setHpfSlope(kSlopes[static_cast<std::size_t>(idx)]);
  } else if (parameterID == ParamIDs::directLpfFreq) {
    directEngine_.setLpfFreq(v);
  } else if (parameterID == ParamIDs::directLpfQ) {
    directEngine_.setLpfQ(v);
  } else if (parameterID == ParamIDs::directLpfSlope) {
    directEngine_.setLpfSlope(kSlopes[static_cast<std::size_t>(idx)]);
  } else if (parameterID == ParamIDs::directThreshold) {
    directMode_.transientDetector_.setThresholdDb(v);
  } else if (parameterID == ParamIDs::directHold) {
    directMode_.transientDetector_.setHoldMs(v);
  } else if (parameterID == ParamIDs::directLookAhead) {
    setLookAheadMs(kLookAheadMs[static_cast<std::size_t>(idx)]);
  } else if (parameterID == ParamIDs::directGain) {
    directEngine_.setGainDb(v);
  } else if (parameterID == ParamIDs::directMute) {
    channelState_.setMute(ChannelState::Channel::direct, v >= 0.5f);
  } else if (parameterID == ParamIDs::directSolo) {
    channelState_.setSolo(ChannelState::Channel::direct, v >= 0.5f);
  }
  // ── Master ──
  else if (parameterID == ParamIDs::masterGain) {
    master_.setGain(v);
  }
}

const juce::String // NOSONAR: JUCE API
BoomBabyAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool BoomBabyAudioProcessor::acceptsMidi() const { return true; }

bool BoomBabyAudioProcessor::producesMidi() const { return false; }

bool BoomBabyAudioProcessor::isMidiEffect() const { return false; }

double BoomBabyAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int BoomBabyAudioProcessor::getNumPrograms() { return 1; }

int BoomBabyAudioProcessor::getCurrentProgram() { return 0; }

void BoomBabyAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String // NOSONAR: JUCE API
BoomBabyAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void BoomBabyAudioProcessor::changeProgramName(int index,
                                               const juce::String &newName) {
  juce::ignoreUnused(index, newName);
}

void BoomBabyAudioProcessor::prepareToPlay(double sampleRate,
                                           int samplesPerBlock) {
  subEngine_.prepareToPlay(sampleRate, samplesPerBlock);
  clickEngine_.prepareToPlay(sampleRate, samplesPerBlock);
  directEngine_.prepareToPlay(sampleRate, samplesPerBlock);
  // sampleMode_ の初期値（false = Input モード）に合わせて passthroughMode_
  // を同期。 未同期のまま triggerNote() が呼ばれるとサンプル未ロード判定で
  // early return し active_ が立たず、renderPassthrough が amp=0
  // で無音になるのを防ぐ。
  directEngine_.setPassthroughMode(!directMode_.sampleMode_.load());
  channelState_.resetDetectors();

  directMode_.transientDetector_.prepare(sampleRate);
  directMode_.transientDetector_.setThresholdDb(-24.0f);
  directMode_.transientDetector_.setHoldMs(50.0f);
  monoMixBuffer_.resize(static_cast<std::size_t>(samplesPerBlock));

  // ルックアヘッド: 最大 6ms 分のキャパシティ（192kHz で 1152 サンプル）
  {
    const auto maxDelaySamples =
        static_cast<int>(std::ceil(0.006 * sampleRate));
    juce::dsp::ProcessSpec spec{};
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;
    directMode_.lookAheadDelay_.setMaximumDelayInSamples(maxDelaySamples + 1);
    directMode_.lookAheadDelay_.prepare(spec);
    directMode_.lookAheadDelay_.reset();
    // 既存の lookAheadSamples_ が有効ならレイテンシーを通知
    const int laSamples = directMode_.lookAheadSamples_.load();
    if (laSamples > 0 && !directMode_.sampleMode_.load())
      setLatencySamples(laSamples);
    else
      setLatencySamples(0);
  }

  inputMonitor_.data_.assign(static_cast<std::size_t>(InputMonitor::kCapacity),
                             0.0f);
  inputMonitor_.fifo_.reset();

  // 全エンジン初期化後に APVTS 値で DSP を復元。
  // setStateInformation が先に呼ばれても prepareToPlay のハードコード値に
  // 上書きされるため、ここで改めて全パラメータを適用する。
  for (const auto *id : {ParamIDs::subWaveShape,   ParamIDs::subLength,
                         ParamIDs::subSatClipType, ParamIDs::subTone1,
                         ParamIDs::subTone2,       ParamIDs::subTone3,
                         ParamIDs::subTone4,       ParamIDs::subGain,
                         ParamIDs::subMute,        ParamIDs::subSolo,
                         ParamIDs::clickMode,      ParamIDs::clickNoiseDecay,
                         ParamIDs::clickBpf1Freq,  ParamIDs::clickBpf1Q,
                         ParamIDs::clickBpf1Slope, ParamIDs::clickSamplePitch,
                         ParamIDs::clickDrive,     ParamIDs::clickClipType,
                         ParamIDs::clickHpfFreq,   ParamIDs::clickHpfQ,
                         ParamIDs::clickHpfSlope,  ParamIDs::clickLpfFreq,
                         ParamIDs::clickLpfQ,      ParamIDs::clickLpfSlope,
                         ParamIDs::clickGain,      ParamIDs::clickMute,
                         ParamIDs::clickSolo,      ParamIDs::directMode,
                         ParamIDs::directPitch,    ParamIDs::directDrive,
                         ParamIDs::directClipType, ParamIDs::directHpfFreq,
                         ParamIDs::directHpfQ,     ParamIDs::directHpfSlope,
                         ParamIDs::directLpfFreq,  ParamIDs::directLpfQ,
                         ParamIDs::directLpfSlope, ParamIDs::directThreshold,
                         ParamIDs::directHold,     ParamIDs::directLookAhead,
                         ParamIDs::directGain,     ParamIDs::directMute,
                         ParamIDs::directSolo,     ParamIDs::masterGain})
    parameterChanged(id, apvts_.getRawParameterValue(id)->load());

  // prepareToPlay の reset() で白紙になった LUT を再ベイク
  bakeAllLutsFromState();
}

void BoomBabyAudioProcessor::setDirectSampleMode(bool isSample) noexcept {
  directMode_.sampleMode_.store(isSample);
  directEngine_.setPassthroughMode(!isSample);
  // パススルーモード時はトランジェント検出を自動有効化
  directMode_.transientDetector_.setEnabled(!isSample);
  // Sample モードではルックアヘッド不要 → レイテンシー 0
  if (isSample)
    setLatencySamples(0);
  else if (const int la = directMode_.lookAheadSamples_.load(); la > 0)
    setLatencySamples(la);
}

void BoomBabyAudioProcessor::setLookAheadMs(float ms) noexcept {
  const double sr = getSampleRate();
  if (sr <= 0.0)
    return;
  const auto samples = static_cast<int>(std::round(ms * 0.001 * sr));
  directMode_.lookAheadSamples_.store(samples);
  // Passthrough 時のみ DAW にレイテンシー通知
  if (!directMode_.sampleMode_.load())
    setLatencySamples(samples);
  else
    setLatencySamples(0);
}

void BoomBabyAudioProcessor::releaseResources() {
  // Currently no resources to release - will be populated when adding DSP
  // processing
}

bool BoomBabyAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
    return false;

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;

  return true;
}

// ─────────────────────────────────────────────────────────────────────────
// processBlock ヘルパー（フリー関数: クラスメソッドカウントに含まれない）
// 将来の拡張（input gain, FFT, latency comp, LUFS 等）は各関数へ追記する
// ─────────────────────────────────────────────────────────────────────────
namespace {

/// 3エンジンをまとめる集約（パラメータ数削減のため）
struct EngineRefs {
  SubEngine &sub;
  ClickEngine &click;
  DirectEngine &direct;
};

/// パススルーモード時: モノミックス → トランジェント検出 → FIFO 供給
void processPassthroughMonitor(BoomBabyAudioProcessor::DirectMode &dm,
                               BoomBabyAudioProcessor::InputMonitor &im,
                               EngineRefs eng,
                               const juce::AudioBuffer<float> &buffer,
                               std::vector<float> &monoMixBuf, int numSamples) {
  if (dm.sampleMode_.load() || buffer.getNumChannels() == 0)
    return;

  auto *mono = monoMixBuf.data();
  const float *ch0 = buffer.getReadPointer(0);
  if (buffer.getNumChannels() >= 2) {
    const float *ch1 = buffer.getReadPointer(1);
    for (int i = 0; i < numSamples; ++i)
      mono[i] = (ch0[i] + ch1[i]) * 0.5f;
  } else {
    std::copy_n(ch0, numSamples, mono);
  }

  if (dm.transientDetector_.isEnabled() &&
      dm.transientDetector_.process(
          std::span<const float>(mono, static_cast<std::size_t>(numSamples)))) {
    eng.sub.triggerNote();
    eng.click.triggerNote();
    eng.direct.triggerNote();
  }

  const int toWrite = juce::jmin(numSamples, im.fifo_.getFreeSpace());
  if (toWrite > 0) {
    int s1;
    int sz1;
    int s2;
    int sz2;
    im.fifo_.prepareToWrite(toWrite, s1, sz1, s2, sz2);
    if (sz1 > 0)
      std::copy_n(mono, sz1, im.data_.data() + s1);
    if (sz2 > 0)
      std::copy_n(mono + sz1, sz2, im.data_.data() + s2);
    im.fifo_.finishedWrite(sz1 + sz2);
  }
}

/// Direct エンジンのパススルー vs サンプルモード呼び分け
void renderDirectEngine(const BoomBabyAudioProcessor::DirectMode &dm,
                        const ChannelState::Passes &passes,
                        DirectEngine &directEng,
                        const std::vector<float> &monoMixBuf,
                        juce::AudioBuffer<float> &buffer, int numSamples,
                        double sr) {
  if (!dm.sampleMode_.load() && passes.direct) {
    directEng.renderPassthrough(
        buffer,
        std::span<const float>(monoMixBuf.data(),
                               static_cast<std::size_t>(numSamples)),
        numSamples, sr);
  } else {
    directEng.render(buffer, numSamples, passes.direct, sr);
  }
}

/// マスター L/R + 各チャンネルのレベル計測
void measureChannelLevels(const ChannelState::Passes &passes,
                          BoomBabyAudioProcessor::MasterSection &master,
                          ChannelState &channelState, EngineRefs eng,
                          const juce::AudioBuffer<float> &buffer,
                          int numSamples) {
  for (std::size_t ch = 0; ch < 2; ++ch) {
    const int bufCh =
        juce::jmin(static_cast<int>(ch), buffer.getNumChannels() - 1);
    master.detector_[ch].process(
        bufCh >= 0 ? buffer.getReadPointer(bufCh) : nullptr, numSamples);
  }

  using enum ChannelState::Channel;
  channelState.detector(sub).process(
      passes.sub ? eng.sub.scratchData() : nullptr, numSamples);
  channelState.detector(click).process(
      passes.click ? eng.click.scratchData() : nullptr, numSamples);
  channelState.detector(direct).process(
      passes.direct ? eng.direct.scratchData() : nullptr, numSamples);
}

} // namespace

void BoomBabyAudioProcessor::handleMidiEvents(juce::MidiBuffer &midiMessages,
                                              int numSamples) {
  // GUI鍵盤のMIDIイベントをバッファにマージ
  keyboardState.processNextMidiBuffer(midiMessages, 0, numSamples, true);

  for (const auto metadata : midiMessages) {
    const auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      subEngine_.triggerNote();
      clickEngine_.triggerNote();
      directEngine_.triggerNote();
    }
    // NoteOff は無視: One-shot 長さは subLengthMs で決定
  }
}

void BoomBabyAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  const auto passes = channelState_.computePasses();
  const int numSamples = buffer.getNumSamples();
  const double sr = getSampleRate();

  // パススルーモード: モノミックス / トランジェント検出 / FIFO 供給
  processPassthroughMonitor(directMode_, inputMonitor_,
                            {subEngine_, clickEngine_, directEngine_}, buffer,
                            monoMixBuffer_, numSamples);

  // ルックアヘッド: monoMixBuffer_ を遅延させて Direct パススルーに渡す
  // （トランジェント検出は即座の入力で完了済み）
  if (const int laSamples = directMode_.lookAheadSamples_.load();
      !directMode_.sampleMode_.load() && laSamples > 0) {
    auto &dl = directMode_.lookAheadDelay_;
    auto *mono = monoMixBuffer_.data();
    for (int i = 0; i < numSamples; ++i) {
      dl.pushSample(0, mono[i]);
      mono[i] = dl.popSample(0, static_cast<float>(laSamples));
    }
  }

  // Direct ミュート: 入力信号を消去（Sub はこの後加算。renderPassthrough も
  // addSample）
  buffer.clear();

  handleMidiEvents(midiMessages, numSamples);
  subEngine_.render(buffer, numSamples, passes.sub, sr);
  clickEngine_.render(buffer, numSamples, passes.click, sr);
  renderDirectEngine(directMode_, passes, directEngine_, monoMixBuffer_, buffer,
                     numSamples, sr);

  // マスターゲイン適用
  buffer.applyGain(juce::Decibels::decibelsToGain(master_.gainDb_.load()));

  measureChannelLevels(passes, master_, channelState_,
                       {subEngine_, clickEngine_, directEngine_}, buffer,
                       numSamples);
}

bool BoomBabyAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *BoomBabyAudioProcessor::createEditor() {
  // clang-format off
  return new BoomBabyAudioProcessorEditor(*this); // NOSONAR: DAW host takes ownership
  // clang-format on
}

void BoomBabyAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  auto state = apvts_.copyState();
  auto xml = state.createXml();
  copyXmlToBinary(*xml, destData);
}

void BoomBabyAudioProcessor::setStateInformation(
    const void *data, // NOSONAR: JUCE API
    int sizeInBytes) {
  if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
    if (xml->hasTagName(apvts_.state.getType())) {
      apvts_.replaceState(juce::ValueTree::fromXml(*xml));

      // replaceState は parameterChanged を発火しないため、
      // 全パラメータの DSP セッターを手動で呼び出す
      for (const auto *id :
           {ParamIDs::subWaveShape,   ParamIDs::subLength,
            ParamIDs::subSatClipType, ParamIDs::subTone1,
            ParamIDs::subTone2,       ParamIDs::subTone3,
            ParamIDs::subTone4,       ParamIDs::subGain,
            ParamIDs::subMute,        ParamIDs::subSolo,
            ParamIDs::clickMode,      ParamIDs::clickNoiseDecay,
            ParamIDs::clickBpf1Freq,  ParamIDs::clickBpf1Q,
            ParamIDs::clickBpf1Slope, ParamIDs::clickSamplePitch,
            ParamIDs::clickDrive,     ParamIDs::clickClipType,
            ParamIDs::clickHpfFreq,   ParamIDs::clickHpfQ,
            ParamIDs::clickHpfSlope,  ParamIDs::clickLpfFreq,
            ParamIDs::clickLpfQ,      ParamIDs::clickLpfSlope,
            ParamIDs::clickGain,      ParamIDs::clickMute,
            ParamIDs::clickSolo,      ParamIDs::directMode,
            ParamIDs::directPitch,    ParamIDs::directDrive,
            ParamIDs::directClipType, ParamIDs::directHpfFreq,
            ParamIDs::directHpfQ,     ParamIDs::directHpfSlope,
            ParamIDs::directLpfFreq,  ParamIDs::directLpfQ,
            ParamIDs::directLpfSlope, ParamIDs::directThreshold,
            ParamIDs::directHold,     ParamIDs::directLookAhead,
            ParamIDs::directGain,     ParamIDs::directMute,
            ParamIDs::directSolo,     ParamIDs::masterGain})
        parameterChanged(id, apvts_.getRawParameterValue(id)->load());

      // エンベロープ LUT を保存済み状態から再ベイク
      bakeAllLutsFromState();
    }
  }
}

void BoomBabyAudioProcessor::bakeAllLutsFromState() {
  const float subLenMs =
      apvts_.getRawParameterValue(ParamIDs::subLength)->load();
  const float directDecayMs =
      apvts_.getRawParameterValue(ParamIDs::directDecay)->load();

  // apvts_.state の ENVELOPE 子ノードから EnvelopeData を復元するヘルパー。
  // 保存データがなければ APVTS のデフォルト値で 1 点エンベロープを作成。
  auto restoreEnv = [&](const char *name, float apvtsDefault) -> EnvelopeData {
    for (int i = 0; i < apvts_.state.getNumChildren(); ++i) {
      auto child = apvts_.state.getChild(i);
      if (child.hasType(juce::Identifier{"ENVELOPE"}) &&
          child.getProperty(juce::Identifier{"name"}).toString() ==
              juce::String(name)) {
        EnvelopeData env;
        env.setDefaultValue(static_cast<float>(
            child.getProperty(juce::Identifier{"defaultValue"}, apvtsDefault)));
        env.clearPoints();
        for (int j = 0; j < child.getNumChildren(); ++j) {
          auto pt = child.getChild(j);
          if (!pt.hasType(juce::Identifier{"POINT"}))
            continue;
          const float t = pt.getProperty(juce::Identifier{"timeMs"}, 0.0f);
          const float v = pt.getProperty(juce::Identifier{"value"}, 1.0f);
          const float c = pt.getProperty(juce::Identifier{"curve"}, 0.0f);
          env.addPoint(t, v);
          env.setSegmentCurve(static_cast<int>(env.getPoints().size()) - 1, c);
        }
        if (!env.hasPoints())
          env.addPoint(0.0f, env.getDefaultValue());
        return env;
      }
    }
    // 保存データなし: デフォルト 1 点
    EnvelopeData env;
    env.setDefaultValue(apvtsDefault);
    env.addPoint(0.0f, apvtsDefault);
    return env;
  };

  const auto load = [&](const char *id) {
    return apvts_.getRawParameterValue(id)->load();
  };

  // LUT → DSP 単位への変換は Editor 側の knob コールバックと対称にする
  bakeLut(restoreEnv("amp", load(ParamIDs::subAmp) / 100.0f),
          subEngine_.envLut(), subLenMs);
  bakeLut(restoreEnv("freq", load(ParamIDs::subFreq)), subEngine_.freqLut(),
          subLenMs);
  bakeLut(restoreEnv("dist", load(ParamIDs::subSatDrive) / 24.0f),
          subEngine_.distLut(), subLenMs);
  bakeLut(restoreEnv("mix", load(ParamIDs::subMix) / 100.0f),
          subEngine_.mixLut(), subLenMs);
  bakeLut(restoreEnv("clickAmp", load(ParamIDs::clickSampleAmp) / 100.0f),
          clickEngine_.clickAmpLut(), subLenMs);
  bakeLut(restoreEnv("directAmp", load(ParamIDs::directAmp) / 100.0f),
          directEngine_.directAmpLut(), directDecayMs);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new BoomBabyAudioProcessor(); // NOSONAR: DAW host takes ownership
}
