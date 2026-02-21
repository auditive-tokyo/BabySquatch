#include "OomphOscillator.h"
#include <cmath>
#include <juce_core/juce_core.h>

void OomphOscillator::prepareToPlay(double newSampleRate) {
  sampleRate = newSampleRate;
  currentIndex = 0.0f;
  buildWavetable();
}

void OomphOscillator::buildWavetable() {
  wavetable.resize(tableSize + 1);
  for (int i = 0; i < tableSize; ++i) {
    wavetable[static_cast<size_t>(i)] = static_cast<float>(
        std::sin(juce::MathConstants<double>::twoPi * i / tableSize));
  }
  wavetable[static_cast<size_t>(tableSize)] = wavetable[0]; // wrap用
}

void OomphOscillator::setNote(int midiNoteNumber) {
  if (midiNoteNumber < 0) {
    currentNote = -1;
    tableDelta = 0.0f;
    currentIndex = 0.0f;
    return;
  }

  currentNote = midiNoteNumber;
  // MIDI ノート → Hz: 440 * 2^((note - 69) / 12)
  const double freq = 440.0 * std::pow(2.0, (midiNoteNumber - 69) / 12.0);
  tableDelta = static_cast<float>(freq * tableSize / sampleRate);
}

float OomphOscillator::getNextSample() {
  if (currentNote < 0)
    return 0.0f;

  const auto index0 = static_cast<size_t>(currentIndex);
  const auto index1 = index0 + 1;
  const float frac = currentIndex - static_cast<float>(index0);

  // 線形補間
  const float sample =
      wavetable[index0] + frac * (wavetable[index1] - wavetable[index0]);

  currentIndex += tableDelta;
  if (currentIndex >= static_cast<float>(tableSize))
    currentIndex -= static_cast<float>(tableSize);

  return sample;
}
