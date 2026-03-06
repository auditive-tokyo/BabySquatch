#pragma once

#include "ChannelFader.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

// ────────────────────────────────────────────────────
// 1 つのパネル = タイトル + ChannelFader（メーター＋フェーダー一体）+ M/S ボタン
// ────────────────────────────────────────────────────
class PanelComponent : public juce::Component {
public:
  PanelComponent(const juce::String &name, juce::Colour accentColour);
  ~PanelComponent() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // フェーダースライダーへの参照（PluginEditor から配線）
  juce::Slider &getFader();

  // Mute / Solo コールバック設定
  void setOnMuteChanged(std::function<void(bool)> cb);
  void setOnSoloChanged(std::function<void(bool)> cb);

  // 外部からボタン状態を更新（Solo ハイライト用）
  void setMuteState(bool muted);
  void setSoloState(bool soloed);

  // レベルメーター: dB 値を返すコールバックを設定
  void setLevelProvider(std::function<float()> provider);

private:
  ChannelFader channelFader;
  juce::Label titleLabel;
  juce::TextButton muteButton{"M"};
  juce::TextButton soloButton{"S"};
  std::function<void(bool)> onMuteChanged;
  std::function<void(bool)> onSoloChanged;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelComponent)
};
