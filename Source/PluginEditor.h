#pragma once

#include "PluginProcessor.h"
#include "GUI/PanelComponent.h"

class BabySquatchAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit BabySquatchAudioProcessorEditor (BabySquatchAudioProcessor&);
    ~BabySquatchAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BabySquatchAudioProcessor& processorRef;

    PanelComponent oomphPanel { "OOMPH",
                                UIConstants::Colours::oomphArc,
                                UIConstants::Colours::oomphThumb };

    PanelComponent clickPanel { "CLICK",
                                UIConstants::Colours::clickArc,
                                UIConstants::Colours::clickThumb };

    PanelComponent dryPanel   { "DRY",
                                UIConstants::Colours::dryArc,
                                UIConstants::Colours::dryThumb };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BabySquatchAudioProcessorEditor)
};
