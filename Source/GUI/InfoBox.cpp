#include "InfoBox.h"
#include "UIConstants.h"

InfoBox::InfoBox() { setInterceptsMouseClicks(false, false); }

void InfoBox::paint(juce::Graphics &g) {
  if (currentText_.isEmpty())
    return;

  g.setColour(UIConstants::Colours::labelText.withAlpha(0.85f));
  g.setFont(juce::Font(juce::FontOptions(11.0f)));

  const auto area = getLocalBounds().reduced(4, 2);
  g.drawFittedText(currentText_, area, juce::Justification::centredLeft, 3);
}

void InfoBox::pollMouseHover() {
  const auto mousePos = juce::Desktop::getInstance().getMousePosition();

  // このコンポーネントのトップレベル親（PluginEditor）内を走査
  auto *top = getTopLevelComponent();
  if (top == nullptr)
    return;

  auto *comp = top->getComponentAt(top->getLocalPoint(nullptr, mousePos));

  // コンポーネント階層を上へ辿り、"info" プロパティを持つ最初のものを探す
  juce::String text;
  while (comp != nullptr && comp != top) {
    const auto &props = comp->getProperties();
    if (props.contains("info")) {
      text = props["info"].toString();
      break;
    }
    comp = comp->getParentComponent();
  }

  if (text != currentText_) {
    currentText_ = text;
    repaint();
  }
}
