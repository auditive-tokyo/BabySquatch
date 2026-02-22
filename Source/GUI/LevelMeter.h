#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

/// 縦型レベルメーター（CPU paint() ベース）。
/// 自前の Timer で 30fps ポーリングし、自動的に repaint() する。
/// setLevelProvider() でdB値を返すコールバックを設定して使う。
class LevelMeter : public juce::Component, private juce::Timer {
public:
  LevelMeter();
  ~LevelMeter() override;

  /// dB 値を返すコールバックを設定（UIスレッドから呼ばれる）。
  void setLevelProvider(std::function<float()> provider);

  void paint(juce::Graphics &g) override;

private:
  void timerCallback() override;

  std::function<float()> levelProvider;
  float displayDb{-100.0f};

  static constexpr float minDb = -48.0f;
  static constexpr float maxDb = 6.0f;
  static constexpr int timerHz = 30;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
