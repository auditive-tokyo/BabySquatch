#include "PanelComponent.h"
#include "UIConstants.h"

// ────────────────────────────────────────────────────
// コンストラクタ
// ────────────────────────────────────────────────────
PanelComponent::PanelComponent(const juce::String &name,
                               juce::Colour accentColour)
    : channelFader(accentColour) {
  // ── ChannelFader（メーター＋フェーダー一体） ──
  addAndMakeVisible(channelFader);

  // ── タイトルラベル ──
  titleLabel.setText(name, juce::dontSendNotification);
  titleLabel.setJustificationType(juce::Justification::centred);
  titleLabel.setColour(juce::Label::textColourId, UIConstants::Colours::text);
  addAndMakeVisible(titleLabel);

  // ── Mute ボタン ──
  muteButton.setClickingTogglesState(true);
  muteButton.setLookAndFeel(&muteButtonLAF_);
  muteButton.setColour(juce::TextButton::textColourOffId,
                       UIConstants::Colours::labelText);
  muteButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
  muteButton.onClick = [this] {
    if (onMuteChanged)
      onMuteChanged(muteButton.getToggleState());
  };
  addAndMakeVisible(muteButton);

  // ── Solo ボタン ──
  soloButton.setClickingTogglesState(true);
  soloButton.setLookAndFeel(&soloButtonLAF_);
  soloButton.setColour(juce::TextButton::textColourOffId,
                       UIConstants::Colours::labelText);
  soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
  soloButton.onClick = [this] {
    if (onSoloChanged)
      onSoloChanged(soloButton.getToggleState());
  };
  addAndMakeVisible(soloButton);
}

// ────────────────────────────────────────────────────
// paint / resized
// ────────────────────────────────────────────────────
void PanelComponent::paint(juce::Graphics &g) {
  g.setColour(UIConstants::Colours::panelBg);
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
}

void PanelComponent::resized() {
  auto area = getLocalBounds().reduced(UIConstants::panelPadding);

  titleLabel.setBounds(area.removeFromTop(UIConstants::labelHeight));

  // 下部ボタン行: M | S
  auto bottomRow = area.removeFromBottom(UIConstants::expandButtonHeight);
  const int btnW = bottomRow.getWidth() / 2;
  muteButton.setBounds(bottomRow.removeFromLeft(btnW));
  soloButton.setBounds(bottomRow);

  // ChannelFader: メーターとフェーダーを内包（内部レイアウトは
  // ChannelFader::resized が管理）
  channelFader.setBounds(area.removeFromLeft(UIConstants::meterWidth +
                                             ChannelFader::faderHandleWidth));
}

// ────────────────────────────────────────────────
// getFader
// ────────────────────────────────────────────────
juce::Slider &PanelComponent::getFader() { return channelFader.getFader(); }

// ────────────────────────────────────────────────
// Mute / Solo コールバック + 外部状態更新
// ────────────────────────────────────────────────
void PanelComponent::setOnMuteChanged(std::function<void(bool)> cb) {
  onMuteChanged = std::move(cb);
}

void PanelComponent::setOnSoloChanged(std::function<void(bool)> cb) {
  onSoloChanged = std::move(cb);
}

void PanelComponent::setMuteState(bool muted) {
  muteButton.setToggleState(muted, juce::dontSendNotification);
}

void PanelComponent::setSoloState(bool soloed) {
  soloButton.setToggleState(soloed, juce::dontSendNotification);
}

void PanelComponent::setLevelProvider(std::function<float()> provider) {
  channelFader.setLevelProvider(std::move(provider));
}
