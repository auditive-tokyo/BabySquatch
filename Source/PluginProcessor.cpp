#include "PluginProcessor.h"
#include "PluginEditor.h"

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
  juce::ignoreUnused(samplesPerBlock);
  oomphOsc.prepareToPlay(sampleRate);
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
    keyboardState.processNextMidiBuffer(midiMessages, 0,
                                        buffer.getNumSamples(), true);

    // MIDIバッファを走査 → OomphOscillator に通知
    for (const auto metadata : midiMessages) {
      const auto msg = metadata.getMessage();
      if (msg.isNoteOn()) {
        oomphOsc.setNote(msg.getNoteNumber());
      } else if (msg.isNoteOff() &&
                 msg.getNoteNumber() == oomphOsc.getCurrentNote()) {
        oomphOsc.setNote(-1);
      }
    }
  }

  // サイン波を出力バッファに加算
  if (oomphOsc.isActive()) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    for (int sample = 0; sample < numSamples; ++sample) {
      const float oscSample = oomphOsc.getNextSample();
      for (int ch = 0; ch < numChannels; ++ch) {
        buffer.addSample(ch, sample, oscSample);
      }
    }
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
