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

void OomphOscillator::triggerNote() {
  active = true;
  currentIndex = 0.0f;
}

void OomphOscillator::stopNote() {
  active = false;
  tableDelta = 0.0f;
  currentIndex = 0.0f;
}

void OomphOscillator::setFrequencyHz(float hz) {
  tableDelta = static_cast<float>(hz * tableSize / sampleRate);
}

float OomphOscillator::getNextSample() {
  if (!active)
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
