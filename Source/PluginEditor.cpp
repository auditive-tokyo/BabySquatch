#include "PluginProcessor.h"
#include "PluginEditor.h"

BabySquatchAudioProcessorEditor::BabySquatchAudioProcessorEditor (BabySquatchAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    addAndMakeVisible (oomphPanel);
    addAndMakeVisible (clickPanel);
    addAndMakeVisible (dryPanel);

    setSize (UIConstants::windowWidth, UIConstants::windowHeight);
}

BabySquatchAudioProcessorEditor::~BabySquatchAudioProcessorEditor()
{
}

void BabySquatchAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (UIConstants::Colours::background);
}

void BabySquatchAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (UIConstants::panelPadding);
    const int panelWidth = (area.getWidth() - UIConstants::panelGap * 2) / 3;

    oomphPanel.setBounds (area.removeFromLeft (panelWidth));
    area.removeFromLeft (UIConstants::panelGap);

    clickPanel.setBounds (area.removeFromLeft (panelWidth));
    area.removeFromLeft (UIConstants::panelGap);

    dryPanel.setBounds (area);
}
