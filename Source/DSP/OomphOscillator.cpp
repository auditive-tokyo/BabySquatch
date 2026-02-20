#include "OomphOscillator.h"
#include <cmath>
#include <juce_core/juce_core.h>

void OomphOscillator::prepareToPlay(double newSampleRate) {
  sampleRate = newSampleRate;
  phase = 0.0;
}

void OomphOscillator::setNote(int midiNoteNumber) {
  if (midiNoteNumber < 0) {
    currentNote = -1;
    phaseIncrement = 0.0;
    phase = 0.0;
    return;
  }

  currentNote = midiNoteNumber;
  // MIDI ノート → Hz: 440 * 2^((note - 69) / 12)
  const double freq = 440.0 * std::pow(2.0, (midiNoteNumber - 69) / 12.0);
  phaseIncrement = freq / sampleRate;
}

float OomphOscillator::getNextSample() {
  if (currentNote < 0)
    return 0.0f;

  const auto sample =
      static_cast<float>(std::sin(phase * juce::MathConstants<double>::twoPi));

  phase += phaseIncrement;
  if (phase >= 1.0)
    phase -= 1.0;

  return sample;
}
