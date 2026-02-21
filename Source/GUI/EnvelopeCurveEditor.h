#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class EnvelopeData;

/// AMP Envelope カーブエディタ
///
/// sin(t) × envelope(t) をオフライン計算して描画する。
/// ポイントなし → フラット波形（defaultValue）
/// ポイントあり → Catmull-Rom スプライン補間されたエンベロープ波形
class EnvelopeCurveEditor : public juce::Component {
public:
    explicit EnvelopeCurveEditor(EnvelopeData& data);

    void paint(juce::Graphics& g) override;

    /// 表示するサイン波のサイクル数を設定
    void setDisplayCycles(float cycles);

    /// エンベロープの表示期間（ms）を設定
    void setDisplayDurationMs(float ms);
    float getDisplayDurationMs() const { return displayDurationMs; }

    /// ポイント変更時コールバック（LUT ベイク等に使用）
    void setOnChange(std::function<void()> cb);

    // ── マウス操作（Phase 2） ──
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    // ── 座標変換ヘルパー ──
    float timeMsToX(float timeMs) const;
    float valueToY(float value) const;
    float xToTimeMs(float x) const;
    float yToValue(float y) const;

    /// ピクセル空間でのヒット判定（-1: なし）
    int findPointAtPixel(float px, float py) const;

    static constexpr float pointHitRadius = 8.0f;

    EnvelopeData& envelopeData;
    float displayDurationMs = 300.0f;
    float displayCycles = 4.0f;
    int dragPointIndex{-1};
    std::function<void()> onChange;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeCurveEditor)
};
