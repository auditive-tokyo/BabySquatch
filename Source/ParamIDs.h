#pragma once

// ── パラメータ ID 定数 ──
// APVTS に登録する全パラメータの ID をここに集約。
// UI (Attachment) と DSP (Listener) の両方から参照する。
namespace ParamIDs {

// ── Sub ──
inline constexpr const char *subWaveShape = "sub_wave_shape";
inline constexpr const char *subLength = "sub_length";
inline constexpr const char *subAmp = "sub_amp";
inline constexpr const char *subFreq = "sub_freq";
inline constexpr const char *subMix = "sub_mix";
inline constexpr const char *subSatDrive = "sub_sat_drive";
inline constexpr const char *subSatClipType = "sub_sat_clip_type";
inline constexpr const char *subTone1 = "sub_tone1";
inline constexpr const char *subTone2 = "sub_tone2";
inline constexpr const char *subTone3 = "sub_tone3";
inline constexpr const char *subTone4 = "sub_tone4";
inline constexpr const char *subGain = "sub_gain";
inline constexpr const char *subMute = "sub_mute";
inline constexpr const char *subSolo = "sub_solo";

// ── Click ──
inline constexpr const char *clickMode = "click_mode";
inline constexpr const char *clickNoiseDecay = "click_noise_decay";
inline constexpr const char *clickBpf1Freq = "click_bpf1_freq";
inline constexpr const char *clickBpf1Q = "click_bpf1_q";
inline constexpr const char *clickBpf1Slope = "click_bpf1_slope";
inline constexpr const char *clickSamplePitch = "click_sample_pitch";
inline constexpr const char *clickSampleAmp = "click_sample_amp";
inline constexpr const char *clickSampleDecay = "click_sample_decay";
inline constexpr const char *clickDrive = "click_drive";
inline constexpr const char *clickClipType = "click_clip_type";
inline constexpr const char *clickHpfFreq = "click_hpf_freq";
inline constexpr const char *clickHpfQ = "click_hpf_q";
inline constexpr const char *clickHpfSlope = "click_hpf_slope";
inline constexpr const char *clickLpfFreq = "click_lpf_freq";
inline constexpr const char *clickLpfQ = "click_lpf_q";
inline constexpr const char *clickLpfSlope = "click_lpf_slope";
inline constexpr const char *clickGain = "click_gain";
inline constexpr const char *clickMute = "click_mute";
inline constexpr const char *clickSolo = "click_solo";

// ── Direct ──
inline constexpr const char *directMode = "direct_mode";
inline constexpr const char *directPitch = "direct_pitch";
inline constexpr const char *directAmp = "direct_amp";
inline constexpr const char *directDrive = "direct_drive";
inline constexpr const char *directClipType = "direct_clip_type";
inline constexpr const char *directDecay = "direct_decay";
inline constexpr const char *directHpfFreq = "direct_hpf_freq";
inline constexpr const char *directHpfQ = "direct_hpf_q";
inline constexpr const char *directHpfSlope = "direct_hpf_slope";
inline constexpr const char *directLpfFreq = "direct_lpf_freq";
inline constexpr const char *directLpfQ = "direct_lpf_q";
inline constexpr const char *directLpfSlope = "direct_lpf_slope";
inline constexpr const char *directThreshold = "direct_threshold";
inline constexpr const char *directHold = "direct_hold";
inline constexpr const char *directGain = "direct_gain";
inline constexpr const char *directMute = "direct_mute";
inline constexpr const char *directSolo = "direct_solo";

// ── Master ──
inline constexpr const char *masterGain = "master_gain";

} // namespace ParamIDs
