#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

// ────────────────────────────────────────────────────────────────
// ChannelFader
//   Sub / Click / Direct の各チャンネル用コンポーネント。
//   縦型レベルメーター（Timer ポーリング描画）と
//   フェーダースライダー（◁ サム）を一体管理し、
//   dB スケール定数を唯一の場所で保持する。
//
//   レイアウト（resized 内部で完結):
//     [ dBラベル | メーターバー | ◁フェーダーハンドル ]
//     ← ─── meterArea ────── →←  faderHandleWidth  →
// ────────────────────────────────────────────────────────────────
class ChannelFader : public juce::Component, private juce::Timer {
public:
  explicit ChannelFader(juce::Colour accentColour);
  ~ChannelFader() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &) override;

  /// フェーダースライダーへの参照（onValueChange 配線などに使用）
  juce::Slider &getFader() { return fader; }

  /// dB 値を返すコールバックを設定（UI スレッドから呼ばれる）
  void setLevelProvider(std::function<float()> provider);

  /// アクセントカラーを更新（ピーク値テキスト色・フェーダーサム色）
  void setAccentColour(juce::Colour colour);

  // ── dB スケール定数（フェーダー range と完全一致） ──
  static constexpr float minDb = -60.0f;
  static constexpr float maxDb = 12.0f;

  /// フェーダーハンドルの幅（PanelComponent::resized から参照）
  static constexpr int faderHandleWidth = 12;
  /// フェーダー値ラベルの高さ（コンポーネント上端に確保）
  static constexpr int valueLabelHeight = 36;

private:
  // ── フェーダーサム LookAndFeel ──
  // トラック透明・◁ 三角でサムを表現
  class FaderLAF : public juce::LookAndFeel_V4 {
  public:
    explicit FaderLAF(juce::Colour accent) : accent_(accent) {}
    void drawLinearSlider(juce::Graphics &, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos,
                          float maxSliderPos, juce::Slider::SliderStyle,
                          juce::Slider &) override;
    int getSliderThumbRadius(juce::Slider &) override { return 0; }
    void setAccent(juce::Colour c) { accent_ = c; }

  private:
    juce::Colour accent_;
  };

  void timerCallback() override;

  FaderLAF faderLAF;
  juce::Slider fader;

  // ── メーター状態 ──
  std::function<float()> levelProvider;
  float displayDb{-100.0f};
  juce::Colour accentColour;

  // ── ピークホールド ──
  float peakDb{minDb}; // minDb と同値で起動時は非表示
  int peakHoldFrames{0};
  float peakFallVelocity{0.0f};

  static constexpr int timerHz = 30;
  static constexpr int peakHoldFrames_ = timerHz * 1; // 1 秒保持
  static constexpr float peakFallPerFrame =
      (maxDb - minDb) / (timerHz * 0.5f); // 0.5 秒でフォール

  // ── 値ラベル直接入力 ──
  void showValueEditor();
  void commitValueEditor();
  juce::TextEditor valueEditor;
  bool isEditing{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelFader)
};
