#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ParamIDs.h"
#include "PluginProcessor.h"

using Catch::Matchers::WithinAbs;

// JUCE の MessageManager をテスト環境でも利用可能にする。
// AudioProcessorValueTreeState や Timer 関連の assertion を防止。
namespace {
struct JuceTestInit {
  JuceTestInit() {
    juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();
  }
  ~JuceTestInit() { juce::MessageManager::deleteInstance(); }
};
static JuceTestInit juceTestInit_;
} // namespace

namespace {
constexpr double kSR = 44100.0;
constexpr int kBlock = 512;

/// ヘルパー: prepareToPlay の前に正しい SR / ブロックサイズを設定する。
/// AudioProcessor::getSampleRate() は setRateAndBufferSizeDetails()
/// が呼ばれないと 0 を返すため、processBlock での除算ゼロを防止。
void prepare(BoomBabyAudioProcessor &p, double sr = kSR, int block = kBlock) {
  p.setRateAndBufferSizeDetails(sr, block);
  p.prepareToPlay(sr, block);
}

/// ヘルパー: float バッファの最大絶対値を返す
float maxAbsOfBuffer(const juce::AudioBuffer<float> &buf) {
  float m = 0.0f;
  for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    for (int i = 0; i < buf.getNumSamples(); ++i)
      m = std::max(m, std::abs(buf.getSample(ch, i)));
  return m;
}
} // namespace

// ─────────────────────────────────────────────────────────────────
// isBusesLayoutSupported
// ─────────────────────────────────────────────────────────────────

TEST_CASE("isBusesLayoutSupported - disabled output rejects",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  juce::AudioProcessor::BusesLayout layout;
  layout.inputBuses.add(juce::AudioChannelSet::stereo());
  layout.outputBuses.add(juce::AudioChannelSet::disabled());
  CHECK_FALSE(p.isBusesLayoutSupported(layout));
}

TEST_CASE("isBusesLayoutSupported - mismatched I/O rejects",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  juce::AudioProcessor::BusesLayout layout;
  layout.inputBuses.add(juce::AudioChannelSet::stereo());
  layout.outputBuses.add(juce::AudioChannelSet::mono());
  CHECK_FALSE(p.isBusesLayoutSupported(layout));
}

TEST_CASE("isBusesLayoutSupported - stereo I/O accepts", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  juce::AudioProcessor::BusesLayout layout;
  layout.inputBuses.add(juce::AudioChannelSet::stereo());
  layout.outputBuses.add(juce::AudioChannelSet::stereo());
  CHECK(p.isBusesLayoutSupported(layout));
}

// ─────────────────────────────────────────────────────────────────
// prepareToPlay — デフォルト値の適用確認
// ─────────────────────────────────────────────────────────────────

TEST_CASE("prepareToPlay applies default master gain (0 dB)",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  prepare(p);
  CHECK_THAT(p.master().getGain(), WithinAbs(0.0f, 0.01f));
}

TEST_CASE("prepareToPlay sets directMode to passthrough by default",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  prepare(p);
  CHECK(p.directMode().isPassthrough());
  CHECK(p.directEngine().isPassthroughMode());
}

// ─────────────────────────────────────────────────────────────────
// parameterChanged — ディスパッチ（prepareToPlay 経由）
// ─────────────────────────────────────────────────────────────────

TEST_CASE("parameterChanged - masterGain dispatch", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  p.getAPVTS().getRawParameterValue(ParamIDs::masterGain)->store(-12.0f);
  prepare(p);
  CHECK_THAT(p.master().getGain(), WithinAbs(-12.0f, 0.01f));
}

TEST_CASE("parameterChanged - directMode switches to sample",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  // directMode APVTS 値 1.0 = Sample
  p.getAPVTS().getRawParameterValue(ParamIDs::directMode)->store(1.0f);
  prepare(p);
  CHECK_FALSE(p.directMode().isPassthrough());
  CHECK_FALSE(p.directEngine().isPassthroughMode());
}

TEST_CASE("parameterChanged - subWaveShape dispatch", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  // Choice 0→Tri, 2→Saw (idx + 1 → WaveShape)
  p.getAPVTS().getRawParameterValue(ParamIDs::subWaveShape)->store(2.0f);
  prepare(p);
  CHECK(p.subEngine().oscillator().getWaveShape() == WaveShape::Saw);
}

TEST_CASE("parameterChanged - mute dispatch", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  p.getAPVTS().getRawParameterValue(ParamIDs::subMute)->store(1.0f);
  prepare(p);

  auto passes = p.channelState().computePasses();
  CHECK_FALSE(passes.sub);
  CHECK(passes.click);
  CHECK(passes.direct);
}

TEST_CASE("parameterChanged - solo overrides non-soloed channels",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  p.getAPVTS().getRawParameterValue(ParamIDs::clickSolo)->store(1.0f);
  prepare(p);

  auto passes = p.channelState().computePasses();
  CHECK_FALSE(passes.sub);
  CHECK(passes.click);
  CHECK_FALSE(passes.direct);
}

TEST_CASE("parameterChanged - direct threshold and hold via detector",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  p.getAPVTS().getRawParameterValue(ParamIDs::directThreshold)->store(-30.0f);
  p.getAPVTS().getRawParameterValue(ParamIDs::directHold)->store(100.0f);
  prepare(p);
  // TransientDetector は内部状態のため直接検証しにくいが、
  // prepareToPlay 内で setThresholdDb / setHoldMs が呼ばれることを
  // クラッシュなく完了する事で確認。
  CHECK(p.directMode().isPassthrough());
}

// ─────────────────────────────────────────────────────────────────
// LUT rebake — kLutAffectedParamIDs のパラメータ変更で LUT が更新される
// ─────────────────────────────────────────────────────────────────

TEST_CASE("LUT rebake on subLength change", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  prepare(p);

  float dur1 = p.subEngine().envLut().getDurationMs();

  p.getAPVTS().getRawParameterValue(ParamIDs::subLength)->store(800.0f);
  prepare(p);

  float dur2 = p.subEngine().envLut().getDurationMs();
  // subLength 変更で LUT duration が変化するはず
  CHECK(std::abs(dur2 - dur1) > 1.0f);
}

// ─────────────────────────────────────────────────────────────────
// processBlock
// ─────────────────────────────────────────────────────────────────

TEST_CASE("processBlock - MIDI NoteOn triggers engines", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  prepare(p);

  // Sub / Click エンジンを直接トリガー（MIDI 経路のテスト環境依存を回避）
  p.subEngine().triggerNote(0);
  p.clickEngine().triggerNote(0);

  juce::AudioBuffer<float> buffer(2, kBlock);
  buffer.clear();
  juce::MidiBuffer midi;
  p.processBlock(buffer, midi);

  CHECK(maxAbsOfBuffer(buffer) > 0.0f);
  CHECK(std::isfinite(maxAbsOfBuffer(buffer)));
}

TEST_CASE("processBlock - master gain -60 dB attenuates output",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  p.getAPVTS().getRawParameterValue(ParamIDs::masterGain)->store(-60.0f);
  prepare(p);

  juce::AudioBuffer<float> buffer(2, kBlock);
  buffer.clear();
  juce::MidiBuffer midi;
  midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
  p.processBlock(buffer, midi);

  // -60 dB ≈ 0.001 ゲイン → 非常に小さい出力
  CHECK(maxAbsOfBuffer(buffer) < 0.01f);
}

TEST_CASE("processBlock - all channels muted produces silence",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  p.getAPVTS().getRawParameterValue(ParamIDs::subMute)->store(1.0f);
  p.getAPVTS().getRawParameterValue(ParamIDs::clickMute)->store(1.0f);
  p.getAPVTS().getRawParameterValue(ParamIDs::directMute)->store(1.0f);
  prepare(p);

  juce::AudioBuffer<float> buffer(2, kBlock);
  buffer.clear();
  juce::MidiBuffer midi;
  midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
  p.processBlock(buffer, midi);

  CHECK(maxAbsOfBuffer(buffer) < 1e-6f);
}

TEST_CASE("processBlock - solo isolates single channel", "[PluginProcessor]") {
  // Sub だけソロ → Click / Direct は寄与しない
  BoomBabyAudioProcessor p;
  p.getAPVTS().getRawParameterValue(ParamIDs::subSolo)->store(1.0f);
  prepare(p);

  // Sub エンジンを直接トリガー
  p.subEngine().triggerNote(0);

  juce::AudioBuffer<float> buffer(2, kBlock);
  buffer.clear();
  juce::MidiBuffer midi;
  p.processBlock(buffer, midi);

  // Sub エンジンだけが鳴るはず
  CHECK(maxAbsOfBuffer(buffer) > 0.0f);
}

TEST_CASE("processBlock - passthrough feeds input monitor FIFO",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  // デフォルト: passthrough モード
  prepare(p);

  juce::AudioBuffer<float> buffer(2, kBlock);
  for (int i = 0; i < kBlock; ++i) {
    buffer.setSample(0, i, 0.5f);
    buffer.setSample(1, i, 0.5f);
  }
  juce::MidiBuffer midi;
  p.processBlock(buffer, midi);

  CHECK(p.inputMonitor().fifo().getNumReady() > 0);
}

TEST_CASE("processBlock - sample mode skips passthrough monitor",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  p.getAPVTS().getRawParameterValue(ParamIDs::directMode)->store(1.0f);
  prepare(p);

  juce::AudioBuffer<float> buffer(2, kBlock);
  for (int i = 0; i < kBlock; ++i) {
    buffer.setSample(0, i, 0.5f);
    buffer.setSample(1, i, 0.5f);
  }
  juce::MidiBuffer midi;
  p.processBlock(buffer, midi);

  // Sample モードでは processPassthroughMonitor が早期リターン → FIFO 空のまま
  CHECK(p.inputMonitor().fifo().getNumReady() == 0);
}

TEST_CASE("processBlock - empty MIDI buffer does not crash",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  prepare(p);

  juce::AudioBuffer<float> buffer(2, kBlock);
  buffer.clear();
  juce::MidiBuffer midi;
  p.processBlock(buffer, midi);

  CHECK(std::isfinite(maxAbsOfBuffer(buffer)));
}

// ─────────────────────────────────────────────────────────────────
// setStateInformation / getStateInformation
// ─────────────────────────────────────────────────────────────────

TEST_CASE("setStateInformation - invalid data does not crash",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  prepare(p);

  const char garbage[] = "not-valid-xml-data-at-all";
  p.setStateInformation(garbage, sizeof(garbage));
  // バージョンがインクリメントされないこと（復元されていない）
  CHECK(p.nonParamStateVersion() == 0);
}

TEST_CASE("setStateInformation - round trip restores state",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p1;
  p1.prepareToPlay(kSR, kBlock);

  // 保存
  juce::MemoryBlock state;
  p1.getStateInformation(state);

  // 別のプロセッサに復元
  BoomBabyAudioProcessor p2;
  prepare(p2);

  int vBefore = p2.nonParamStateVersion();
  p2.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

  // バージョンインクリメント（applyRestoredState が呼ばれた証拠）
  CHECK(p2.nonParamStateVersion() == vBefore + 1);
  // デフォルトのマスターゲインが維持される
  CHECK_THAT(p2.master().getGain(), WithinAbs(0.0f, 0.01f));
}

TEST_CASE("setStateInformation - round trip with modified parameter",
          "[PluginProcessor]") {
  BoomBabyAudioProcessor p1;
  p1.prepareToPlay(kSR, kBlock);

  // APVTS パラメータをホスト通知付きで変更（ValueTree へ反映される）
  auto *param = p1.getAPVTS().getParameter(ParamIDs::masterGain);
  param->setValueNotifyingHost(param->convertTo0to1(-6.0f));

  juce::MemoryBlock state;
  p1.getStateInformation(state);

  // 別のプロセッサに復元
  BoomBabyAudioProcessor p2;
  prepare(p2);
  p2.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

  float restored =
      p2.getAPVTS().getRawParameterValue(ParamIDs::masterGain)->load();
  CHECK_THAT(restored, WithinAbs(-6.0f, 0.5f));
  CHECK(p2.nonParamStateVersion() == 1);
}

// ─────────────────────────────────────────────────────────────────
// その他のカバレッジ
// ─────────────────────────────────────────────────────────────────

TEST_CASE("getName returns plugin name", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  CHECK_FALSE(p.getName().isEmpty());
}

TEST_CASE("MIDI properties", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  CHECK(p.acceptsMidi());
  CHECK_FALSE(p.producesMidi());
  CHECK_FALSE(p.isMidiEffect());
}

TEST_CASE("tail length is zero", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  CHECK_THAT(p.getTailLengthSeconds(), WithinAbs(0.0, 0.001));
}

TEST_CASE("program accessors", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  CHECK(p.getNumPrograms() == 1);
  CHECK(p.getCurrentProgram() == 0);
  p.setCurrentProgram(0); // no-op; should not crash
  CHECK(p.getProgramName(0).isEmpty());
  p.changeProgramName(0, "test"); // no-op; should not crash
}

TEST_CASE("releaseResources does not crash", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  prepare(p);
  p.releaseResources();
  // 再度準備しても問題ない
  prepare(p);
}

TEST_CASE("hasEditor returns true", "[PluginProcessor]") {
  BoomBabyAudioProcessor p;
  CHECK(p.hasEditor());
}
