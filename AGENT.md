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

## TODO

### Oomph Sine OSC + 波形表示（OpenGL GPU描画）

**目的:** MIDIモード時に鍵盤入力でサイン波を発音し、波形を展開エリアに表示する

**実装ステップ（順番に実施）:**

1. ✅ **`acceptsMidi()` を `true` に変更**（Standalone/DAW共通MIDI経路確立）
   - `CMakeLists.txt`: `NEEDS_MIDI_INPUT TRUE`
   - `PluginProcessor.cpp`: `acceptsMidi()` → `true`
   - `IS_SYNTH` は `FALSE` のまま（Effect扱いを維持）

2. ✅ **`OomphOscillator` 実装**（`Source/DSP/OomphOscillator.h/.cpp`）
   - sine波生成、`setNote(int midiNote)` でHz変換
   - `prepareToPlay(sampleRate)` で初期化
   - `getNextSample()` → オーディオスレッドで呼び出し

3. ✅ **`PluginProcessor::processBlock` でMIDI→OSC接続**
   - `MidiKeyboardState` を Processor に移動（GUI鍵盤→processBlock のMIDI受け渡し）
   - MIDIバッファを走査、noteOn/noteOff → OomphOscillatorに通知
   - 出力バッファにサイン波を加算
   - `KeyboardComponent` は外部から `MidiKeyboardState&` を受け取る形にリファクタリング

4. **FIXEDモード時のMIDI発音を無効化**
   - FIXEDモードはオーディオ入力に対してfixedノートを適用するモード（OSCが音を生成するのではなく、入力オーディオを加工する用途）
   - `processBlock` で `KeyboardComponent::getMode()` を参照し、FIXEDモード時は `keyboardState.processNextMidiBuffer()` をスキップ（GUI鍵盤のMIDIをバッファにマージしない）
   - Processor が Editor の `KeyboardComponent` を参照できるよう `getMode()` を Processor 側に公開する方法を検討（例：`std::atomic<bool> fixedModeActive` を Processor に持ち、Editor から書き込む）

5. **`juce::AbstractFifo` で波形データをスレッドセーフ受け渡し**
   - オーディオスレッド → push サンプル
   - UIスレッド（60Hz Timer） → pop → GPU転送

6. **`WaveformDisplay` 実装**（`Source/GUI/WaveformDisplay.h/.cpp`）
   - `juce::Component` + `juce::OpenGLRenderer` 継承
   - `glContext.setRenderer(this)` + `glContext.attachTo(*this)` + `setContinuousRepainting(true)`
   - `newOpenGLContextCreated()`: GLSL シェーダー + VBO 初期化
   - `renderOpenGL()`: 波形データ → VRAM転送 → 描画（sin はCPU生成値をそのまま使う）
   - `openGLContextClosing()`: リソース解放

7. **PluginEditor に WaveformDisplay 統合**
   - 展開エリアの上部（鍵盤の上）に配置
   - MIDIモード時のみ表示

**注意事項:**
- `paint()` は OpenGL コンポーネントでは使用しない（`renderOpenGL()` のみ）
- 波形表示はCPU側で生成したサンプル列をGPUに転送する方式（GPU側でsineを再計算しない）
- AtomicRingBuffer は `juce::AbstractFifo` を使用（自前実装不要）
- BassSplitter の GPU描画実装を参考にすること
