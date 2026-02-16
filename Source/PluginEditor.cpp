#include "PluginProcessor.h"
#include "PluginEditor.h"

BabySquatchAudioProcessorEditor::BabySquatchAudioProcessorEditor (BabySquatchAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize (400, 300);
}

BabySquatchAudioProcessorEditor::~BabySquatchAudioProcessorEditor()
{
}

void BabySquatchAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);

    g.setColour (juce::Colours::white);
    g.setFont (30.0f);
    g.drawFittedText ("Baby Squatch", getLocalBounds(), juce::Justification::centred, 1);
    
    g.setFont (15.0f);
    auto bounds = getLocalBounds();
    bounds.removeFromTop (bounds.getHeight() / 2 + 20);
    g.drawFittedText ("Hello World!", bounds, juce::Justification::centred, 1);
}

void BabySquatchAudioProcessorEditor::resized()
{
}
