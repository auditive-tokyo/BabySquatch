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
- **AMP Envelope エディタ**（Kick Ninja スタイル）
  - `EnvelopeData`：ヘッダオンリーモデル。制御点なし→`defaultValue`（フラット）、1点→定数、2点→線形補間、3点以上→Catmull-Rom スプライン補間。ファントムポイント端点処理・隣接クランプ `movePoint`
  - `EnvelopeCurveEditor`：`paint()` ベース。`sin(t) × evaluate(t)` でオフライン波形プレビュー。スプラインカーブ＋コントロールポイント描画。左ダブルクリックでポイント追加／削除、ドラッグで移動
  - ロックフリー LUT 統合：Editor が `evaluate()` を 512 点ベイク → `bakeEnvelopeLut()` でダブルバッファにコピー → `std::atomic` フリップ。オーディオスレッドはノートオンで `noteTimeSamples` リセット → サンプル毎に LUT 参照してエンベロープゲイン適用

## Source構成（概要）

`tree` の生出力を貼り付け（更新時は `cd Source && tree` を再実行して置き換える）:

```text
.
├── DSP
│   ├── EnvelopeData.h         // AMP Envelopeデータモデル（Catmull-Rom・ヘッダオンリー）
│   ├── OomphOscillator.cpp    // Oomph用Wavetable OSC実装（波形生成・補間）
│   └── OomphOscillator.h      // Oomph用Wavetable OSC宣言（公開API）
├── GUI
│   ├── CustomSliderLAF.h      // ノブ描画LookAndFeel（グラデーション/値表示）
│   ├── EnvelopeCurveEditor.cpp // エンベロープカーブエディタ実装（paint/マウス操作）
│   ├── EnvelopeCurveEditor.h  // エンベロープカーブエディタ宣言
│   ├── KeyboardComponent.cpp  // 鍵盤UI実装（MIDI/FIXED切替・PCキー入力）
│   ├── KeyboardComponent.h    // 鍵盤UI宣言（モード/固定ノート制御API）
│   ├── PanelComponent.cpp     // OOMPH/CLICK/DRY共通パネル実装
│   ├── PanelComponent.h       // 共通パネル宣言（ノブ・展開ボタン）
│   ├── UIConstants.h          // UI定数集約（色・レイアウト寸法）
│   ├── WaveformDisplay.cpp    // OpenGL波形描画実装（将来用・現在未接続）
│   └── WaveformDisplay.h      // OpenGL波形描画宣言（将来用・現在未接続）
├── PluginEditor.cpp           // Editor実装（レイアウト/UIイベント/LUTベイク配線）
├── PluginEditor.h             // Editor宣言（UI構成とメンバー）
├── PluginProcessor.cpp        // Processor実装（MIDI処理・DSP・LUTエンベロープ）
└── PluginProcessor.h          // Processor宣言（LUTダブルバッファ・AudioProcessorIF）

3 directories, 16 files
```

## TODO

### Phase 3以降（将来の拡張）

- `EnvelopeData pitchEnvData` — ピッチ包絡（`EnvelopeCurveEditor` 再利用）
- `EnvelopeData blendEnvData` — Sine/他ウェーブシェイプのミックス比
- Click/Dry パネルの展開エリアに同様のエディタを配置
- エンベロープの保存／復元（`getStateInformation` / `setStateInformation`）
