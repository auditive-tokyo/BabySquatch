#include "PluginProcessor.h"
#include "PluginEditor.h"

BabySquatchAudioProcessor::BabySquatchAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

BabySquatchAudioProcessor::~BabySquatchAudioProcessor() = default;

const juce::String BabySquatchAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BabySquatchAudioProcessor::acceptsMidi() const
{
    return false;
}

bool BabySquatchAudioProcessor::producesMidi() const
{
    return false;
}

bool BabySquatchAudioProcessor::isMidiEffect() const
{
    return false;
}

double BabySquatchAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BabySquatchAudioProcessor::getNumPrograms()
{
    return 1;
}

int BabySquatchAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BabySquatchAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String BabySquatchAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void BabySquatchAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void BabySquatchAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void BabySquatchAudioProcessor::releaseResources()
{
}

bool BabySquatchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void BabySquatchAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                        juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer, midiMessages);
    juce::ScopedNoDenormals noDenormals;

    // シンプルなパススルー（何もしない）
    // 将来的にここにキックエンハンス処理を追加
}

bool BabySquatchAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BabySquatchAudioProcessor::createEditor()
{
    return new BabySquatchAudioProcessorEditor (*this);
}

void BabySquatchAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void BabySquatchAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BabySquatchAudioProcessor();
}
