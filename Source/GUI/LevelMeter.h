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

  /// ピーク値テキストのアクセントカラーを設定。
  void setAccentColour(juce::Colour colour) { accentColour = colour; }

  void paint(juce::Graphics &g) override;
  void mouseDown(const juce::MouseEvent &) override; // クリックでピークリセット

private:
  void timerCallback() override;

  std::function<float()> levelProvider;
  float displayDb{-100.0f};
  juce::Colour accentColour{0xFFCCCCCC};

  // ── ピークホールド ──
  float peakDb{-100.0f};        // 表示中のピーク値
  int peakHoldFrames{0};        // 保持残り frames
  float peakFallVelocity{0.0f}; // フォール中の速度 (dB/frame)

  static constexpr float minDb = -48.0f;
  static constexpr float maxDb = 6.0f;
  static constexpr int timerHz = 30;
  // 3秒保持, その後 2秒かけて minDb まで落とす
  static constexpr int peakHoldFrames_ = timerHz * 3;
  static constexpr float peakFallPerFrame = (maxDb - minDb) / (timerHz * 2.0f);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
