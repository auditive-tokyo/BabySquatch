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

## ビルド環境

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

- 3パネルUI（OOMPH / CLICK / DRY）、各カラー付きロータリーノブ、Mute（グレー/赤）/ Solo（グレー/黄）トグルボタン、展開ボタン（▼）を下部に配置
- 各チャンネルノブ左に縦型レベルメーター（`LevelMeter` + `LevelDetector`）
  - DSP: `LevelDetector`（ヘッダオンリー）— `std::atomic<float>` ピーク計測、バリスティクス付き減衰
  - GUI: `LevelMeter`— `paint()` ベース、`juce::Timer` 30fps ポーリング、緑→黄→赤 グラデーション、-48dB～+6dB スケール、dBスケール目盛（0/-12/-24/-36/-48）
- **クラス分割リファクタリング**（SonarQube S1448対応）
  - `ChannelState`（ヘッダオンリー）— Channel enum、Mute/Solo アトミック配列、LevelDetector配列、`computePasses()`
  - `EnvelopeLutManager`（ヘッダオンリー）— LUTダブルバッファ、`bake()`、`getActiveLut()`、`setDurationMs()`
  - `BabySquatchAudioProcessor` はこれらに委譲（メソッド数 36→28 に削減）
- グラデーションアーク（指数的グロー）
- ノブ中央にdB値表示、クリックでキーボード入力、ダブルクリックで0.0dBリセット
- 展開パネル（per-channel ▼ボタン、共有展開エリア）
- MIDI鍵盤（KeyboardComponent）
  - C0〜C7表示、PCキー演奏対応（A=C2ベース）
  - Z/Xでオクターブシフト（両モード共通）
  - MIDI / FIXED モード切り替えボタン
  - FIXEDモード：単音固定、同じ鍵盤クリックで解除
- **AMP Envelope エディタ**（Kick Ninja スタイル）
  - `EnvelopeData`：ヘッダオンリーモデル。制御点なし→`defaultValue`（フラット）、1点→定数、2点→線形補間、3点以上→Catmull-Rom スプライン補間。ファントムポイント端点処理・隣接クランプ `movePoint`
  - `EnvelopeCurveEditor`：`paint()` ベース。`sin(t) × evaluate(t)` でオフライン波形プレビュー。スプラインカーブ＋コントロールポイント描画。左ダブルクリックでポイント追加／削除、ドラッグで移動
  - ロックフリー LUT 統合：Editor が `evaluate()` を 512 点ベイク → `bakeEnvelopeLut()` でダブルバッファにコピー → `std::atomic` フリップ。オーディオスレッドはノートオンで `noteTimeSamples` リセット → サンプル毎に LUT 参照してエンベロープゲイン適用

## Pitch / MIDI 設計方針（Kick Ninja準拠）

- **MIDIキーボード = トリガー専用**: どの鍵盤を押しても同じ音が鳴る（note-onイベントとして扱う）
- **ピッチは Pitch Envelope が絶対値で決定**: 例: `F1(43.6Hz) → C1(32.7Hz)` スイープ
- **波形プレビュー**: Pitch Envelope の値に基づいて描画（MIDIノートではなく）
- **FIXEDモード**: この方式ではMIDIノートが音程に影響しないため、FIXED/MIDIモードの区別は不要になる可能性あり（要検討・将来削除候補）
- **現状の `OomphOscillator::setNote(midiNoteNumber)`**: `triggerNote()` に変更予定。ピッチは Pitch Envelope LUT から取得
- **キーボードの役割**: DAWでのMIDIパターン打ち込み（タイミング制御）、Standaloneでの試聴トリガー

## 描画方針

- **現在**: CPU描画（`paint()` ベース）が主体
  - `EnvelopeCurveEditor`, `CustomSliderLAF`, `LevelMeter`（予定）等
- **`WaveformDisplay`**: 生OpenGLシェーダー使用（将来用・**現在未使用**・未接続）
  - 高密度リアルタイム波形描画（数千〜数万ポイント）が必要な場合向け
- **GPU化の切り替え**: `openGLContext.attachTo(*this)` を `PluginEditor` コンストラクタに1行追加するだけで、全ての `paint()` コードがそのままGPU加速される（JUCEの `Graphics` APIがバックエンドを抽象化）
- **結論**: 今は `paint()` で実装し続けて問題なし。将来パフォーマンス要求が上がった場合でも、既存コードの変更なく GPU化可能

## Source構成（概要）

`tree` の生出力を貼り付け（更新時は `cd Source && tree` を再実行して置き換える）:

```text
.
├── DSP
│   ├── ChannelState.h         // チャンネルMute/Solo/レベル管理（ヘッダオンリー、BabySquatchAudioProcessorから委譲）
│   ├── EnvelopeData.h         // AMP Envelopeデータモデル（Catmull-Rom・ヘッダオンリー）
│   ├── EnvelopeLutManager.h   // LUTダブルバッファ管理（ヘッダオンリー、ロックフリーbake/read）
│   ├── LevelDetector.h        // ロックフリーピーク検出（ヘッダオンリー、オーディオスレッド writer / UI reader）
│   ├── OomphOscillator.cpp    // Oomph用Wavetable OSC実装（波形生成・補間）
│   └── OomphOscillator.h      // Oomph用Wavetable OSC宣言（公開API）
├── GUI
│   ├── CustomSliderLAF.h      // ノブ描画LookAndFeel（グラデーション/値表示）
│   ├── EnvelopeCurveEditor.cpp // エンベロープカーブエディタ実装（paint/マウス操作）
│   ├── EnvelopeCurveEditor.h  // エンベロープカーブエディタ宣言
│   ├── KeyboardComponent.cpp  // 鍵盤UI実装（MIDI/FIXED切替・PCキー入力）
│   ├── KeyboardComponent.h    // 鍵盤UI宣言（モード/固定ノート制御API）
│   ├── LevelMeter.cpp         // レベルメーター実装（paint・30fps Timer）
│   ├── LevelMeter.h           // レベルメーター宣言
│   ├── PanelComponent.cpp     // OOMPH/CLICK/DRY共通パネル実装
│   ├── PanelComponent.h       // 共通パネル宣言（ノブ・展開ボタン）
│   ├── UIConstants.h          // UI定数集約（色・レイアウト寸法）
│   ├── WaveformDisplay.cpp    // OpenGL波形描画実装（将来用・現在未接続）
│   └── WaveformDisplay.h      // OpenGL波形描画宣言（将来用・現在未接続）
├── PluginEditor.cpp           // Editor実装（レイアウト/UIイベント/LUTベイク配線）
├── PluginEditor.h             // Editor宣言（UI構成とメンバー）
├── PluginProcessor.cpp        // Processor実装（MIDI処理・DSP・ChannelState/EnvelopeLutManagerへ委譲）
└── PluginProcessor.h          // Processor宣言（AudioProcessorIF・channelState()/envLut()アクセサ）

3 directories, 20 files
```

## TODO

### Phase 3以降（将来の拡張）

- **Oomph パラメータ設計の見直し**
  - 現状: Oomphロータリーノブが `oomphGainDb`（マスターアウト乗算）と `ampEnvData.setDefaultValue()`（AMPエンベロープの`defaultValue`）を同時に書き込んでいる。`defaultValue` はエンベロープにコントロールポイントが**ない**場合のみ `evaluate()` で使われるため、ポイント未設定時はノブが実質的にAMPの振幅も兼ねて制御するが、ポイントが1つ以上設定されると`defaultValue`は無視される
  - 変更方針:
    - **Oomphロータリーノブ** → `oomphGainDb` のみ制御（マスターアウト専用）に整理
    - **Amplitude専用ノブ** → 新設（`EnvelopeData::defaultValue` を制御）
    - 今後の拡張: Pitch、Blend、Amplitudeなど複数のパラメータが追加予定。AMPはその一つという位置付け
  - 実装:
    1. 展開パネル内にAmplitude専用ノブを追加
    2. Pitchエンベロープ用のパラメータ/ノブ追加（`EnvelopeData pitchEnvData` + `EnvelopeCurveEditor`）
    3. Blendエンベロープ用のパラメータ/ノブ追加（Sine/他波形のミックス比）
    4. Oomphロータリーノブは最終段で全体にかかるゲインとして処理

- **KeyboardComponent FIXEDモードのキーボード入力問題**
  - 現状: FIXEDモードでもmacのキーボード入力に反応してしまい、noteが選択されてしまう
  - 期待動作: FIXEDモードではマウス操作のみ有効、キーボード入力は無効化
  - 実装:
    1. `KeyboardComponent` の `keyPressed` / `keyStateChanged` ハンドラーで `isMidiMode` チェック追加
    2. FIXEDモードの場合はキーボードイベントを無視（`return false` でイベントを親に伝播）
    3. または `setWantsKeyboardFocus(isMidiMode)` でフォーカス制御
  - 関連ファイル: `Source/GUI/KeyboardComponent.cpp`

- **CapsLock 中はキーボードフォーカスを常に鍵盤に固定**
  - 現状: 展開パネルを開くか鍵盤をクリックした場合のみ `KeyboardComponent` がフォーカスを持つ。ロータリーノブ操作後などはフォーカスが外れ、PCキーからのMIDI入力が効かなくなる
  - 期待動作: CapsLockがONの間は、他のUI操作（ノブ・ボタン等）をしても常に `KeyboardComponent` がフォーカスを保持する
  - 実装案:
    1. `PluginEditor` の `focusOfChildComponentChanged()` または各コンポーネントの `mouseDown()` をオーバーライドし、CapsLock状態を `juce::ModifierKeys::getCapsLockState()` で確認
    2. CapsLockがONなら即座に `keyboard.grabKeyboardFocus()` を呼び直す
    3. または `KeyboardComponent` 側で `juce::ComponentPeer` レベルのフォーカス監視を行い、自律的に再取得する
  - 関連ファイル: `Source/GUI/KeyboardComponent.cpp`, `Source/PluginEditor.cpp`

- **エンベロープ横軸タイムライン表示**
  - 現状: タイムライン表示なし（軸ラベル・目盛りなし）
  - 追加: **Attack / Body / Decay / Tail** の4セクション境界線 + ラベル表示（Kick Ninja スタイル）
  - 各セクション境界は `EnvelopeCurveEditor` 内で定義可能なメンバ変数（例: `attackEndMs`, `bodyEndMs`, `decayEndMs`）または `EnvelopeData` 側で保持
  - `paint()` 内で縦線 + テキストラベル描画
  - UI的に境界を調整可能にするか（ドラッグで移動）は要検討

- **Band-limited Wavetable 実装（Oomph モジュール拡張: Sine + Square / Triangle / Saw ミックス）**
  - **方針**: Band-limited Wavetable 方式を採用（BLEPではなく）
    - 理由: 複数波形のモーフィング（Sine ↔ Square ↔ Triangle ↔ Saw のミックス）に適している
    - CPU効率が高く、メモリ消費も許容範囲
    - 後段フィルターではエイリアシング除去不可（折り返しノイズは生成前に防ぐ必要あり）
  - 実装:
    1. 周波数帯域ごとに事前計算した複数のテーブル（例: 10オクターブ分）を `std::array<std::vector<float>, N>` で保持
    2. 再生周波数に応じてテーブルを選択 + クロスフェード
    3. Square / Triangle / Saw 各波形で独立したテーブルセット、現在のSine波形とのミックスパラメータ追加
    4. `OomphOscillator` を拡張し、波形ミックス比率 `setWaveformBlend(float sine, float square, float tri, float saw)` を実装
  - 参考: Serum風のWavetable設計、JUCE `dsp::Oscillator` + カスタムテーブル
  - **注意**: 単純なローパスフィルター後付けはエイリアシング除去には無効（音色変化の演出用途のみ）

- **Click モジュール実装**（より高度な処理: 短いトランジェント/ノイズバースト生成等）

- **Pitch Envelope 実装**
  - `EnvelopeData pitchEnvData` — ピッチ包絡（`EnvelopeCurveEditor` 再利用）
  - Y軸: MIDIノート値（またはHz）、X軸: 時間（ms）
  - Pitch Envelope が Osc の周波数を直接制御（MIDIノートは使わない）
  - `OomphOscillator::setNote()` → `triggerNote()` に改名、ピッチは Pitch Envelope LUT から毎サンプル取得
  - Pitch Envelope LUT も AMP と同じロックフリーダブルバッファ方式で実装
  - 波形プレビュー (`EnvelopeCurveEditor`) は Pitch Envelope の値に基づいてサイクル数を可変描画

- **FIXED / MIDI モードの見直し**
  - Kick Ninja準拠方式（トリガー専用）では、全鍵盤が同じ音を鳴らすため FIXED/MIDI の区別が不要
  - キーボードUIの簡素化を検討（モード切替ボタン削除、単一トリガーモード化）
  - または将来的にキーボード自体をオプション表示に変更

- `EnvelopeData blendEnvData` — Sine/他ウェーブシェイプのミックス比
- Click/Dry パネルの展開エリアに同様のエディタを配置
- エンベロープの保存／復元（`getStateInformation` / `setStateInformation`）
