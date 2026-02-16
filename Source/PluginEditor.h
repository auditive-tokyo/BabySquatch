#pragma once

#include "PluginProcessor.h"

class BabySquatchAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit BabySquatchAudioProcessorEditor (BabySquatchAudioProcessor&);
    ~BabySquatchAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BabySquatchAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BabySquatchAudioProcessorEditor)
};
