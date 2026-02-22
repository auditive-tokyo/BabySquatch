#pragma once

#include "DSP/OomphOscillator.h"
#include <array>
#include <atomic>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

class BabySquatchAudioProcessor : public juce::AudioProcessor
{
public:
    BabySquatchAudioProcessor();
    ~BabySquatchAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    
    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override; // NOSONAR: JUCE API signature
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override; // NOSONAR: JUCE API signature
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /// GUI鍵盤との共有 MidiKeyboardState
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    /// FIXEDモード時にMIDI発音を無効化するフラグ（Editor から書き込み）
    void setFixedModeActive(bool active) { fixedModeActive.store(active); }
    bool isFixedModeActive() const { return fixedModeActive.load(); }

    /// Oomph出力ゲイン（dB）— UIノブから書き込み、processBlockで適用
    void setOomphGainDb(float db) { oomphGainDb.store(db); }
    float getOomphGainDb() const { return oomphGainDb.load(); }

    // ── Mute / Solo ──
    enum class Channel { oomph = 0, click = 1, dry = 2 };
    void setMute(Channel ch, bool muted);
    void setSolo(Channel ch, bool soloed);

    // ── エンベロープ LUT（ロックフリー・ダブルバッファ） ──

    static constexpr int envLutSize = 512;

    /// UIスレッドから呼び出し: 非アクティブ側バッファにコピー → アトミックフリップ
    void bakeEnvelopeLut(const float* data, int size);

    /// エンベロープ適用期間（ms）— UI側から設定
    void setEnvDurationMs(float ms) { envDurationMs.store(ms); }
    float getEnvDurationMs() const { return envDurationMs.load(); }

private:
    // ── processBlock ヘルパー ──
    struct ChannelPasses { bool oomph; bool dry; };
    ChannelPasses computeChannelPasses() const;
    void handleMidiEvents(juce::MidiBuffer& midiMessages, int numSamples);
    void renderOomph(juce::AudioBuffer<float>& buffer, int numSamples, bool oomphPass);

    juce::MidiKeyboardState keyboardState;
    OomphOscillator oomphOsc;
    std::atomic<bool> fixedModeActive{false};
    std::atomic<float> oomphGainDb{0.0f};
    std::array<std::atomic<bool>, 3> channelMute{};
    std::array<std::atomic<bool>, 3> channelSolo{};
    std::vector<float> oomphScratchBuffer; // prepareToPlay でリサイズ

    // ── エンベロープ LUT ダブルバッファ ──
    std::array<float, envLutSize> envLut0{};
    std::array<float, envLutSize> envLut1{};
    std::atomic<int> envLutActiveIndex{0};   ///< オーディオスレッドが読む側
    std::atomic<float> envDurationMs{300.0f};
    float noteTimeSamples{0.0f};             ///< ノートオンからの経過サンプル数

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BabySquatchAudioProcessor)
};
