#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class EnvelopeData;

/// AMP Envelope カーブエディタ（Phase 1: フラット波形プレビューのみ）
///
/// sin(t) × envelope(t) をオフライン計算して描画する。
/// Phase 1 では envelope は定数（defaultValue）なので、
/// 一定振幅のサイン波を oomphArc カラーで塗りつぶし表示する。
class EnvelopeCurveEditor : public juce::Component {
public:
    explicit EnvelopeCurveEditor(EnvelopeData& data);

    void paint(juce::Graphics& g) override;

    /// 表示するサイン波のサイクル数を設定
    void setDisplayCycles(float cycles);

    /// 将来の自動再計算用コールバック
    void setOnChange(std::function<void()> cb);

private:
    /// Phase 1 では固定定数。Phase 2 で可変化
    static constexpr float defaultDurationMs = 300.0f;

    EnvelopeData& envelopeData;
    float displayCycles = 4.0f;
    std::function<void()> onChange;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeCurveEditor)
};
