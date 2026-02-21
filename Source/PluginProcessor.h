#pragma once

#include "DSP/OomphOscillator.h"
#include <atomic>
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

private:
    juce::MidiKeyboardState keyboardState;
    OomphOscillator oomphOsc;
    std::atomic<bool> fixedModeActive{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BabySquatchAudioProcessor)
};
