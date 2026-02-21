# BabySquatch Project

## プロジェクト概要

BabySquatchは、Boz LabのSasquatchから自分が使う機能だけを集約したキックエンハンスプラグインです。

## プラグイン構成

BabySquatchは3つのモジュールで構成されています：

1. **Oomph** - サブ周波数の生成
2. **Click** - アタック部分の生成（Square, Triangle, Sawなどの短いノイズ）
3. **Dry** - オリジナルのキック音（入力信号）

これらを組み合わせることで、より豊かで立体的なキックサウンドを実現します。

## 対応フォーマット

- AU
- VST3
- Standalone

## ビルル環境

- macOS
- CMake/Xcode
- C++20
- JUCE Framework

## エージェント動作ルール (必須)

- 変更を加える前に必ずユーザーに確認を取ること。ユーザーの明示的な承認がない限り、コードや設定を変更しない。
- ユーザーが指示したことだけを実行すること。提案や代替案は提示しても、実行はユーザーの許可がある場合のみ行う。
- 推測や勝手な想像でコードを書き換えないこと。意図が不明な場合は必ず質問して確認する。
- 変更を行う際は、どのファイルを、なぜ、どう修正したかを簡潔に報告し、必要ならビルド／確認手順を添えること。
- **コード改修後の必須確認手順:** 変更を行ったら必ず `make check && make lint` を実行し、問題がなければ `make run` で動作確認を行うこと。
- **CMake／ファイル構成変更時の手順:** 新規ファイルの追加・削除、または `CMakeLists.txt` の変更を行った場合は、まず `make cmake` を実行してプロジェクトを再生成し、その後に `make check && make lint` → `make run` の順で確認すること。

## 実装済み機能

- 3パネルUI（OOMPH / CLICK / DRY）、各カラー付きロータリーノブ
- グラデーションアーク（指数的グロー）
- ノブ中央にdB値表示、クリックでキーボード入力、ダブルクリックで0.0dBリセット
- 展開パネル（per-channel ▼ボタン、共有展開エリア）
- MIDI鍵盤（KeyboardComponent）
  - C0〜C7表示、PCキー演奏対応（A=C2ベース）
  - Z/Xでオクターブシフト（両モード共通）
  - MIDI / FIXED モード切り替えボタン
  - FIXEDモード：単音固定、同じ鍵盤クリックで解除

## Source構成（概要）

`tree` の生出力を貼り付け（更新時は `cd Source && tree` を再実行して置き換える）:

```text
.
├── DSP
│   ├── OomphOscillator.cpp   // Oomph用Wavetable OSC実装（波形生成・補間）
│   └── OomphOscillator.h     // Oomph用Wavetable OSC宣言（公開API）
├── GUI
│   ├── CustomSliderLAF.h      // ノブ描画LookAndFeel（グラデーション/値表示）
│   ├── KeyboardComponent.cpp  // 鍵盤UI実装（MIDI/FIXED切替・PCキー入力）
│   ├── KeyboardComponent.h    // 鍵盤UI宣言（モード/固定ノート制御API）
│   ├── PanelComponent.cpp     // OOMPH/CLICK/DRY共通パネル実装
│   ├── PanelComponent.h       // 共通パネル宣言（ノブ・展開ボタン）
│   ├── UIConstants.h          // UI定数集約（色・レイアウト寸法）
│   ├── WaveformDisplay.cpp    // OpenGL波形描画実装（Shader/VBO描画）
│   └── WaveformDisplay.h      // OpenGL波形描画コンポーネント宣言
├── PluginEditor.cpp           // Editor実装（レイアウト/タイマー/UIイベント）
├── PluginEditor.h             // Editor宣言（UI構成とメンバー）
├── PluginProcessor.cpp        // Processor実装（MIDI処理・DSP・FIFO）
└── PluginProcessor.h          // Processor宣言（AudioProcessorインターフェース）

3 directories, 14 files
```

## TODO

### AMP Envelope エディタ実装（Kick Ninja スタイル）

**目的:** OOMPHパネル展開エリアに、ユーザーが制御点を打ってサイン波の振幅包絡を視覚的に編集できるインタラクティブカーブエディタを実装する。

**設計方針:**

- 表示するのはリアルタイム波形ではなく、**パラメータから計算したプレビュー**
  - `sin(t) × envelope(t)` をオフラインで計算 → 描画
- 制御点を打ってCatmull-Romスプライン補間でカーブを生成
- カーブエディタと波形プレビューを**1コンポーネントに一体化**（Kick Ninja スタイル）
- `WaveformDisplay`（OpenGL）はファイルとして残すが、Editorからは外す
- Phase 1はUI完結（DSPへの適用は後のステップ）

**描画レイヤー（`paint()` 内）:**

1. 背景（`UIConstants::Colours::waveformBg`）
2. 波形塗りつぶし（`sin(t) × envelope(t)` + `oomphArc` グラデーション）
3. エンベロープカーブ（スプライン線）
4. コントロールポイント（ドラッグ可能な白い小円）

**マウス操作:**
| 操作 | 動作 |
|------|------|
| 空白を左ダブルクリック | ポイント追加 |
| ポイント上を左ダブルクリック | ポイント削除 |
| ポイントをドラッグ | 移動（横軸: ms, 縦軸: Amplitude 0〜200, 中央=100） |

**状態遷移（Kick Ninja スタイル）:**
- **デフォルト（ポイントなし）** → フラット波形、AMPノブで値調整可能
- **ポイント追加** → Automation モード、複数ポイント間を Catmull-Rom スプライン補間
- **全ポイント削除** → デフォルト状態に戻る

**単位系:**
- `EnvelopeData.defaultValue`: **0.0〜2.0 の線形ゲイン**（0% 〜 200%、中央 1.0 = 100%）
- OOMPHノブ（dB）→ `Decibels::decibelsToGain()` で変換してから `setDefaultValue()` に渡す
- 描画時は `defaultValue × 100` で 0〜200 スケール表示

**実装ステップ:**

### Phase 1: AMP フラット波形 + ノブ連動

**目標:** デフォルト状態（ポイントなし）で、AMPノブの値に応じたフラット波形を表示・編集

1. **`Source/DSP/EnvelopeData.h` 新規作成**（ヘッダオンリー）
   - `EnvelopeData` クラス：定数値 `float defaultValue{1.0f}`（線形ゲイン 0.0〜2.0）のみ
   - `getValue()` → 常に `defaultValue` を返す
   - `setDefaultValue(float v)` / `getDefaultValue()` アクセッサ
   - **`EnvelopePoint` 構造体は Phase 2 で追加**（Phase 1 では不要）

2. **`Source/GUI/EnvelopeCurveEditor.h/.cpp` 新規作成**
   - `juce::Component` 継承（OpenGLなし、`paint()` ベース）
   - コンストラクタ: `EnvelopeCurveEditor(EnvelopeData& data)`
   - `paint()`: 背景 → フラット波形塗りつぶし（`defaultValue` に基づく高さ）
   - `setDisplayCycles(float)` で波の山数制御
   - `displayDurationMs` は Phase 1 では固定定数（`static constexpr float defaultDurationMs = 300.0f`）、可変化は Phase 2
   - `setOnChange(std::function<void()>)` コールバック（将来の自動再計算用）
   - **Phase 1 ではマウス操作なし**（描画のみ）

3. **`CMakeLists.txt` 更新**
   - `EnvelopeData.h`, `EnvelopeCurveEditor.h`, `EnvelopeCurveEditor.cpp` を `target_sources` に追加

4. **`PluginEditor.h` 更新**
   - `#include "GUI/WaveformDisplay.h"` を削除 → `#include "GUI/EnvelopeCurveEditor.h"` / `#include "DSP/EnvelopeData.h"` 追加
   - `WaveformDisplay waveformDisplay` メンバ削除（ファイルは CMakeLists に残す）
   - `waveformTransferBuffer` メンバ削除
   - `juce::Timer` 継承を削除（`timerCallback` / `startTimerHz` / `stopTimer` も除去）
   - `updateWaveformVisibility()` → `updateEnvelopeEditorVisibility()` にリネーム
   - `EnvelopeData ampEnvData` メンバ追加
   - `EnvelopeCurveEditor envelopeCurveEditor{ampEnvData}` メンバ追加

5. **`PluginEditor.cpp` 更新**
   - コンストラクタ: `addChildComponent(envelopeCurveEditor)`
   - OOMPHノブ `onValueChange`: dB → `Decibels::decibelsToGain()` → `ampEnvData.setDefaultValue(gain)` + `envelopeCurveEditor.repaint()`
   - `requestExpand`: `envelopeCurveEditor.setVisible(isOpen)`
   - `resized` レイアウト順:
     1. `keyboard` → `expandArea.removeFromBottom(keyboardHeight + modeButtonHeight)`
     2. `envelopeCurveEditor` → `expandArea` の残り全域
     3. `expandableArea` → 不使用（Phase 1 では bounds 設定不要、将来チャンネル固有UIの器）
   - `timerCallback` 完全削除（Timer 継承ごと除去）

6. **`PluginProcessor.cpp` 更新**
   - `pushWaveformBlock()` 呼び出しをコメントアウト（Editor が pop しないため FIFO が満杯で空振り続ける。コード自体は残す）

7. **`make cmake` → `make check && make lint` → `make run` で動作確認**

### Phase 2: Automation（複数ポイント制御）

**目標:** ポイント追加・削除・ドラッグで複数ポイント Automation を作成、Catmull-Rom スプライン補間で滑らかにつなぐ

- `EnvelopeData` に `EnvelopePoint { float time, value; }` 構造体と `std::vector<EnvelopePoint> points` 追加
- `addPoint` / `removePoint` / `movePoint` / `evaluate(t)` 実装
- `EnvelopeCurveEditor` に `mouseDown` / `mouseDrag` / `mouseUp` でポイント操作追加
- `paint()` 拡張: スプライン線 + コントロールポイント描画
- `displayDurationMs` を可変に（ズーム/パン対応）
- `PluginProcessor` に `EnvelopeData ampEnvData` を移し、`processBlock` でゲイン適用
- Timer/FIFO を必要に応じて復活

**将来の拡張（Phase 3以降）:**
- `EnvelopeData pitchEnvData` — ピッチ包絡（`EnvelopeCurveEditor` 再利用）
- `EnvelopeData blendEnvData` — Sine/他ウェーブシェイプのミックス比
- Click/Dry パネルの展開エリアに同様のエディタを配置（CLICK: 時間軸と高さ制御、DRY: パス制御など）