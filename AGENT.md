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

### Oomph Wavetable OSC + 波形表示（OpenGL GPU描画）

**目的:** MIDIモード時に鍵盤入力でWavetable方式のサイン波を発音し、波形を展開エリアに表示する

**実装ステップ（順番に実施）:**

1. ✅ **`acceptsMidi()` を `true` に変更**（Standalone/DAW共通MIDI経路確立）
   - `CMakeLists.txt`: `NEEDS_MIDI_INPUT TRUE`
   - `PluginProcessor.cpp`: `acceptsMidi()` → `true`
   - `IS_SYNTH` は `FALSE` のまま（Effect扱いを維持）

2. ✅ **`OomphOscillator` 実装**（`Source/DSP/OomphOscillator.h/.cpp`）
   - sine波生成（`std::sin` 直接呼び出し版）、`setNote(int midiNote)` でHz変換
   - `prepareToPlay(sampleRate)` で初期化
   - `getNextSample()` → オーディオスレッドで呼び出し

3. ✅ **`PluginProcessor::processBlock` でMIDI→OSC接続**
   - `MidiKeyboardState` を Processor に移動（GUI鍵盤→processBlock のMIDI受け渡し）
   - MIDIバッファを走査、noteOn/noteOff → OomphOscillatorに通知
   - 出力バッファにサイン波を加算
   - `KeyboardComponent` は外部から `MidiKeyboardState&` を受け取る形にリファクタリング

4. ✅ **`OomphOscillator` を Wavetable 方式に切り替え**
   - 外部API（`prepareToPlay`, `setNote`, `getNextSample`, `isActive`, `getCurrentNote`）は変更なし
   - 内部を `std::sin()` 毎サンプル呼び出し → テーブル読み出し＋線形補間に置き換え
   - `PluginProcessor.cpp` の変更は不要（API不変のため）
     > 詳細: 後述「OomphOscillator Wavetable化 実装メモ」を参照

5. ✅ **FIXEDモード時のMIDI発音を無効化**
   - FIXEDモードはオーディオ入力に対してfixedノートを適用するモード（OSCが音を生成するのではなく、入力オーディオを加工する用途）
   - `processBlock` で `KeyboardComponent::getMode()` を参照し、FIXEDモード時は `keyboardState.processNextMidiBuffer()` をスキップ（GUI鍵盤のMIDIをバッファにマージしない）
     > Processor が Editor の `KeyboardComponent` を参照できるよう `getMode()` を Processor 側に公開する方法を検討（例：`std::atomic<bool> fixedModeActive` を Processor に持ち、Editor から書き込む）

6. ✅ **`juce::AbstractFifo` で波形データをスレッドセーフ受け渡し**
   - オーディオスレッド → push サンプル
   - UIスレッド（60Hz Timer） → pop → GPU転送
     > 注意: AtomicRingBuffer は使わず `juce::AbstractFifo` を使用（自前実装不要）

7. ✅ **`WaveformDisplay` 実装**（`Source/GUI/WaveformDisplay.h/.cpp`）
   - `juce::Component` + `juce::OpenGLRenderer` 継承
   - `glContext.setRenderer(this)` + `glContext.attachTo(*this)` + `setContinuousRepainting(true)`
   - `newOpenGLContextCreated()`: GLSL シェーダー + VBO 初期化
   - `renderOpenGL()`: 波形データ → VRAM転送 → 描画（Wavetableで生成したサンプル値をそのまま使う）
   - `openGLContextClosing()`: リソース解放
     > 注意: OpenGLコンポーネントでは `paint()` は使わず `renderOpenGL()` を使用
     > 注意: 波形はCPU側で生成したサンプル列をGPUへ転送し、GPU側でsineを再計算しない

8. **PluginEditor に WaveformDisplay 統合**
   - 展開エリアの上部（鍵盤の上）に配置
   - MIDIモード時のみ表示
   - OOMPHロータリースライダーを Oomph出力レベルに紐づけ（最終段のボリュームとして適用）
     > 方針: Oomphチェーン（Wavetable OSC → 将来ADSR）の最後でゲイン適用し、チャンネル出力直前の最終音量をノブで制御する
     > 実装案: ノブ値はUIでdB表示、Processor側で `juce::Decibels::decibelsToGain()` によりlinear gainへ変換して適用（DSP責務はProcessorに集約）
     > 参考: BassSplitter の GPU描画実装を参照すること

---

## OomphOscillator Wavetable化 実装メモ（方針決定済み）

### 決定事項

**→ Wavetable方式に切り替える（現行の `std::sin()` Oscillatorを置き換え）**

タイミングとして、Step 5（AbstractFifo）・Step 6（WaveformDisplay）実装前のこのタイミングが最もリファクタリングコストが低い。
GPUパイプラインを繋いでしまうと `getNextSample()` への依存が深まり、後から変えにくくなるため今やる。

### なぜ Wavetable か（技術的根拠）

#### 現行コードの問題点

`OomphOscillator.cpp` の `getNextSample()` は毎サンプル `std::sin()` を呼んでいる：

```cpp
std::sin(phase * juce::MathConstants<double>::twoPi)
```

`std::sin()` はCPU的にコストの高い超越関数。現在はOomph単体で問題ないが、
ADSR・ポリフォニー・Clickモジュールの波形追加が進むと無視できなくなる。

#### Wavetableの仕組み（概要）

Wavetableとは「1周期分の波形を配列（テーブル）として事前に計算しておき、
再生時はそのテーブルをインデックスで読み出す」方式。

```
初期化時（prepareToPlay）:
  テーブルサイズ = 2048サンプル（+ wrap用に末尾に先頭と同じ値を追加 → 計2049要素）
  for i in 0..2047:
    table[i] = sin(2π * i / 2048)
  table[2048] = table[0]  // 線形補間用のwrap

再生時（getNextSample）:
  index0 = (int)currentIndex
  index1 = index0 + 1
  frac = currentIndex - index0        // 小数部分（0.0〜1.0）
  sample = table[index0] + frac * (table[index1] - table[index0])  // 線形補間
  currentIndex += tableDelta
  if (currentIndex >= tableSize): currentIndex -= tableSize
```

`tableDelta` の計算：

```
tableDelta = frequency * tableSize / sampleRate
```

例: 44100Hz サンプルレート、C2（65.41Hz）、テーブルサイズ2048の場合:

```
tableDelta = 65.41 * 2048 / 44100 ≈ 3.04
```

→ 毎サンプル3.04インデックス分進む = 65.41Hzの波形を再生

#### BabySquatch固有の利点

| 観点                               | 現行 (std::sin)                    | Wavetable                                          |
| ---------------------------------- | ---------------------------------- | -------------------------------------------------- |
| CPU負荷（ランタイム）              | 毎サンプル超越関数呼び出し         | テーブル読み出し＋補間のみ（軽い）                 |
| エイリアシング（Oomph/サブ帯域）   | 実用上ほぼ問題なし                 | 同様に問題なし（サブは高調波少ない）               |
| エイリアシング（Click/Square/Saw） | **問題あり**（折り返し雑音が出る） | **対応可能**（Band-limited tableを用意すれば解決） |
| 波形拡張性                         | 波形ごとに別クラス必要             | テーブルを差し替えるだけで波形切り替え可能         |
| WaveformDisplay連携                | getNextSample()の戻り値をFifoへ    | 同じ。インターフェースは変わらない                 |

→ 特に **Click モジュール（Square/Triangle/Saw）への拡張**を見据えると、
今Wavetable基盤を作っておくと後がシンプルになる。

### 実装方針（変更スコープを最小化）

**ポイント：外部インターフェース（API）は一切変えない。内部実装だけ差し替える。**

これにより `PluginProcessor.cpp` の変更は不要。Step 1〜3の既実装も壊れない。

#### 変更ファイル

- `Source/DSP/OomphOscillator.h` — privateメンバを変更
- `Source/DSP/OomphOscillator.cpp` — 実装を全面置き換え

#### `OomphOscillator.h` の変更点

```cpp
// 変更前（privateメンバ）
int currentNote = -1;
double sampleRate = 44100.0;
double phase = 0.0;
double phaseIncrement = 0.0;

// 変更後（privateメンバ）
static constexpr int tableSize = 2048;
std::vector<float> wavetable;   // tableSize + 1 要素（wrap用）

int currentNote = -1;
double sampleRate = 44100.0;
float currentIndex = 0.0f;
float tableDelta = 0.0f;        // phaseIncrementに相当

void buildWavetable();          // prepareToPlay()内で呼び出す
```

#### `OomphOscillator.cpp` の変更点

```cpp
void OomphOscillator::prepareToPlay(double newSampleRate) {
  sampleRate = newSampleRate;
  currentIndex = 0.0f;
  buildWavetable();
}

void OomphOscillator::buildWavetable() {
  wavetable.resize(tableSize + 1);
  for (int i = 0; i < tableSize; ++i)
    wavetable[i] = std::sin(2.0f * juce::MathConstants<float>::pi * i / tableSize);
  wavetable[tableSize] = wavetable[0];  // wrap用
}

void OomphOscillator::setNote(int midiNoteNumber) {
  if (midiNoteNumber < 0) {
    currentNote = -1;
    tableDelta = 0.0f;
    currentIndex = 0.0f;
    return;
  }
  currentNote = midiNoteNumber;
  const double freq = 440.0 * std::pow(2.0, (midiNoteNumber - 69) / 12.0);
  tableDelta = static_cast<float>(freq * tableSize / sampleRate);
}

float OomphOscillator::getNextSample() {
  if (currentNote < 0)
    return 0.0f;

  const int index0 = static_cast<int>(currentIndex);
  const int index1 = index0 + 1;
  const float frac = currentIndex - index0;

  // 線形補間
  const float sample = wavetable[index0] + frac * (wavetable[index1] - wavetable[index0]);

  currentIndex += tableDelta;
  if (currentIndex >= static_cast<float>(tableSize))
    currentIndex -= static_cast<float>(tableSize);

  return sample;
}
```

### 実装後の確認手順

1. `make cmake`（ファイル変更はなし、CMakeLists.txt も変更なし → 不要かもしれないがひとまず実行）
2. `make check && make lint`
3. `make run` で Standalone 起動、鍵盤を弾いてサイン波が鳴るか確認
4. 音質に違和感がないか（ブツ切れ・クリックノイズ・音程ズレがないか）確認

### 将来の波形拡張メモ（Click モジュール向け）

Clickで Square/Saw を追加する際は `wavetable` を差し替えるだけで対応可能：

```cpp
// Square波テーブル生成例（Band-limited版は別途検討）
for (int i = 0; i < tableSize; ++i)
  wavetable[i] = (i < tableSize / 2) ? 1.0f : -1.0f;
wavetable[tableSize] = wavetable[0];
```

ただしSquare/Sawはエイリアシング対策（Band-limited Wavetable or BLEP）が別途必要。
その際は `buildWavetable()` をパラメータ付きに拡張する方針で対応する。
