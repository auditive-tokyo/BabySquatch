#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

inline void saveModeStateFromWidgets(const auto &clk, auto &dst) {
  dst.hpfFreq = clk.hpf.slider.getValue();
  dst.hpfQ = clk.hpf.qSlider.getValue();
  dst.hpfSlope = clk.hpf.slope.getSlope();
  dst.lpfFreq = clk.lpf.slider.getValue();
  dst.lpfQ = clk.lpf.qSlider.getValue();
  dst.lpfSlope = clk.lpf.slope.getSlope();
  dst.drive = clk.noise.saturator.driveSlider.getValue();
  dst.clipType = clk.noise.saturator.clipType.getSelected();
}

inline void restoreModeStateToWidgets(auto &clk, const auto &src, auto &eng) {
  clk.hpf.slider.setValue(src.hpfFreq, juce::dontSendNotification);
  clk.hpf.qSlider.setValue(src.hpfQ, juce::dontSendNotification);
  clk.hpf.slope.setSlope(src.hpfSlope);
  clk.lpf.slider.setValue(src.lpfFreq, juce::dontSendNotification);
  clk.lpf.qSlider.setValue(src.lpfQ, juce::dontSendNotification);
  clk.lpf.slope.setSlope(src.lpfSlope);
  clk.noise.saturator.driveSlider.setValue(src.drive,
                                           juce::dontSendNotification);
  clk.noise.saturator.clipType.setSelected(src.clipType);
  eng.setHpfFreq(static_cast<float>(src.hpfFreq));
  eng.setHpfQ(static_cast<float>(src.hpfQ));
  eng.setHpfSlope(src.hpfSlope);
  eng.setLpfFreq(static_cast<float>(src.lpfFreq));
  eng.setLpfQ(static_cast<float>(src.lpfQ));
  eng.setLpfSlope(src.lpfSlope);
  eng.setDriveDb(static_cast<float>(src.drive));
  eng.setClipType(src.clipType);
}
