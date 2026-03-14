#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/// 固定配置のパラメーター説明エリア。
/// マウス下のコンポーネントの "info" プロパティを読み取り表示する。
/// PluginEditor の timerCallback から pollMouseHover() を呼ぶこと。
class InfoBox : public juce::Component {
public:
  InfoBox();

  void paint(juce::Graphics &g) override;

  /// timerCallback
  /// から呼ばれ、マウス下のコンポーネントを走査して表示テキストを更新
  void pollMouseHover();

  /// コンポーネントに InfoBox 用テキストを設定するヘルパー
  static void setInfo(juce::Component &comp, const juce::String &text) {
    comp.getProperties().set("info", text);
  }

private:
  juce::String currentText_;
};
