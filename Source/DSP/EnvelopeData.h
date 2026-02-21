#pragma once

/// Amplitude Envelope データモデル（Phase 1: フラット定数値のみ）
///
/// - defaultValue: 線形ゲイン 0.0〜2.0（1.0 = 100% = OOMPHノブ 0.0dB）
/// - Phase 2 で EnvelopePoint 配列と Catmull-Rom 補間を追加予定
class EnvelopeData {
public:
    /// デフォルト値を取得（Phase 1 では常にこの値を返す）
    float getValue() const { return defaultValue; }

    /// OOMPHノブ側で Decibels::decibelsToGain() 変換済みの値を渡す
    void setDefaultValue(float v) { defaultValue = v; }
    float getDefaultValue() const { return defaultValue; }

private:
    /// 線形ゲイン: 0.0〜2.0、デフォルト 1.0（= 100%、ノブ 0.0dB 相当）
    float defaultValue{1.0f};
};
