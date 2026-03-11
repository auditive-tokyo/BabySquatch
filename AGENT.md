# BoomBaby Project

## プロジェクト概要

BoomBabyは、Boz LabのSasquatchとHim DSPのKick Ninjaから自分が使う機能だけを集約したキックエンハンスプラグインです。

## プラグイン構成

BoomBabyは3つのモジュールで構成されています：

1. **Sub** - サブ周波数の生成
2. **Click** - アタック部分の生成（Noise / Sample）
3. **Direct** - オリジナルのキック音（入力信号）またはSample Load

これらを組み合わせることで、より豊かで立体的なキックサウンドを実現します。

## 対応フォーマット

- AU
- VST3
- Standalone

## ビルド環境

- macOS
- CMake/Xcode
- C++23
- JUCE Framework

## エージェント動作ルール (必須)

- 変更を加える前に必ずユーザーに確認を取ること。ユーザーの明示的な承認がない限り、コードや設定を変更しない。
- ユーザーが指示したことだけを実行すること。提案や代替案は提示しても、実行はユーザーの許可がある場合のみ行う。
- 推測や勝手な想像でコードを書き換えないこと。意図が不明な場合は必ず質問して確認する。
- 変更を行う際は、どのファイルを、なぜ、どう修正したかを簡潔に報告し、必要ならビルド／確認手順を添えること。
- **コード改修後の必須確認手順:** 変更を行ったら必ず `make check && make lint` を実行し、問題がなければ `make run` で動作確認を行うこと。
- **CMake／ファイル構成変更時の手順:** 新規ファイルの追加・削除、または `CMakeLists.txt` の変更を行った場合は、まず `make cmake` を実行してプロジェクトを再生成し、その後に `make check && make lint` → `make run` の順で確認すること。

## 描画方針

- **現在**: CPU描画（`paint()` ベース）が主体
  - `EnvelopeCurveEditor`, `CustomSliderLAF`, `LevelMeter` 等
- **GPU化の切り替え**: `openGLContext.attachTo(*this)` を `PluginEditor` コンストラクタに1行追加するだけで、全ての `paint()` コードがそのままGPU加速される（JUCEの `Graphics` APIがバックエンドを抽象化）
- **結論**: 今は `paint()` で実装し続けて問題なし。将来パフォーマンス要求が上がった場合でも、既存コードの変更なく GPU化可能

## Source構成（概要）

`tree` の生出力を貼り付け（更新時は `cd Source && tree` を再実行して置き換える）:

```text
.
├── DSP
│   ├── ChannelState.h         // チャンネルMute/Solo/レベル管理（ヘッダオンリー）
│   ├── ClickEngine.cpp        // Click DSP 実装（Noise/Sample モード、BPF1カスケード、HPF/LPF）
│   ├── ClickEngine.h          // Click DSP 宣言
│   ├── DirectEngine.cpp       // Direct DSP 実装（入力パススルー / サンプル再生）
│   ├── DirectEngine.h         // Direct DSP 宣言
│   ├── EnvelopeData.h         // エンベロープデータモデル（Catmull-Rom・ヘッダオンリー）
│   ├── EnvelopeLutManager.h   // LUTダブルバッファ管理（ヘッダオンリー、ロックフリー）
│   ├── LevelDetector.h        // ロックフリーピーク検出（ヘッダオンリー）
│   ├── SamplePlayer.cpp       // サンプル再生エンジン実装
│   ├── SamplePlayer.h         // サンプル再生エンジン宣言
│   ├── Saturator.h            // Drive + ClipType 共通 DSP ヘルパー（ヘッダオンリー、Sub/Click/Direct 共用）
│   ├── SubEngine.cpp          // Sub DSP 実装（Wavetable OSC、LUT 駆動）
│   ├── SubEngine.h            // Sub DSP 宣言
│   ├── SubOscillator.cpp      // Sub用Wavetable OSC実装
│   ├── SubOscillator.h        // Sub用Wavetable OSC宣言
│   └── TransientDetector.h    // トランジェント検出（ヘッダオンリー、Auto Trigger 用）
├── GUI
│   ├── ChannelFader.cpp       // チャンネルフェーダー実装（メーター＋フェーダー一体）
│   ├── ChannelFader.h         // チャンネルフェーダー宣言（Sub/Click/Direct 共通）
│   ├── ClickParams.cpp        // Click パネル UI セットアップ / レイアウト
│   ├── CustomSliderLAF.h      // ノブ描画LookAndFeel（グラデーション/値表示）+ CustomSlider（自然スクロール対応）
│   ├── DirectParams.cpp       // Direct パネル UI セットアップ / レイアウト
│   ├── EnvelopeCurveEditor.cpp // エンベロープカーブエディタ実装
│   ├── EnvelopeCurveEditor.h  // エンベロープカーブエディタ宣言
│   ├── KeyboardComponent.cpp  // 鍵盤UI実装
│   ├── KeyboardComponent.h    // 鍵盤UI宣言
│   ├── LutBaker.h             // Sub波形 LUT ベイク処理（ヘッダオンリー）
│   ├── MasterFader.cpp        // マスターフェーダー実装（横向きフェーダー＋L/Rメーター）
│   ├── MasterFader.h          // マスターフェーダー宣言
│   ├── PanelComponent.cpp     // SUB/CLICK/DIRECT共通パネル実装
│   ├── PanelComponent.h       // 共通パネル宣言（ChannelFader・M/S ボタン）
│   ├── SubParams.cpp          // Sub パネル UI セットアップ / レイアウト
│   ├── UIConstants.h          // UI定数集約（色・レイアウト寸法）
│   └── WaveformUtils.h        // 波形プレビュー描画ヘルパー（ClickParams/DirectParams 共通、ヘッダオンリー）
├── PluginEditor.cpp
├── PluginEditor.h
├── PluginProcessor.cpp
└── PluginProcessor.h
```

## TODO

- **マスターリミッター（検討中）**
  - 目的: マスター出力段にブリックウォールリミッターを挿入し、クリッピングを防止
  - 候補方式:
    1. **ソフトニー方式（推奨）**: Attack/Release エンベロープフォロワーでゲインリダクション計算。CPU 負荷ほぼゼロ、~80行。`LevelDetector` を流用可能
    2. **ハードクリップ**: `jlimit()` で振幅を制限。5行で実装可能だが歪みが出る
    3. **Look-ahead 方式**: 高品質だが遅延補正が必要になり複雑度が上がる
  - しきい値: -0.1 dBFS 固定か、MasterFader に Ceiling ノブを追加するか要検討
  - 実装箇所: `PluginProcessor::processBlock()` の `applyGain()` 直後
  - GUI: MasterFader のメーターにゲインリダクション量（GR）表示を追加できる

- **Infobox（ホバー説明 UI）（検討中）**
  - 目的: キーボード右のスペースに固定の説明エリアを設置し、各ノブ・パラメーターにホバーしたときに使い方を表示
  - 実装方針: **中央集権型文字列テーブル + カスタム `InfoBox` コンポーネント**
    - `Source/GUI/InfoBox.h / InfoBox.cpp`: 右端固定の表示コンポーネント
    - `Source/GUI/InfoStrings.h`: ID → 説明文のテーブル（`std::unordered_map`、バイナリ埋め込み）
    - 各コンポーネントに `setComponentID("sub_level")` 等を設定し、汎用 `mouseEnter` / `mouseExit` ハンドラで `infoBox.show(id)` / `infoBox.hide()` を呼ぶ
  - 外部ファイル（.json / .txt）方式は配布・パス解決が面倒なので採用しない
  - `juce::TooltipWindow` はスタイル固定で右端常駐 UI に不向きなので採用しない

- **レイテンシー補正（Delay Compensation）**
  - 背景: BoomBaby は Direct（入力パススルー）と Sub/Click（生成信号）を混合するため、どちらかに処理遅延が生じると内部でタイミングズレが発生する
  - 実装方針（2段階）:
    1. **内部補正**: Direct（Dry）側に `juce::dsp::DelayLine` で同量の遅延を与え、Sub/Click（Wet）との位相を揃える
    2. **ホスト報告**: `setLatencySamples(n)` で DAW に総遅延サンプル数を通知し、他トラックとの同期を確保
  - トリガー: ルックアヘッド型トランジェント検出を実装した際に必須となる（ルックアヘッド分だけ Wet が遅れるため）
  - 現状: 大きな遅延源がないため未着手で問題なし。トランジェント検出実装時に同時対応する

- **CI / CD パイプライン構築**
  - 目的: GitHub Actions でビルド・静的解析を自動化し、プッシュごとに品質を担保する
  - 優先タスク:
    1. **`compile_commands.json` の CI 生成**: SonarQube Cloud が Compilation Database モードで解析できるよう、スキャン前に以下を実行するステップを追加
       ```yaml
       - name: Generate compile_commands.json
         run: cmake -S . -B build-clangd -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
       ```
    2. **SonarQube Cloud スキャンステップ**: `sonar.cfamily.compile-commands=build-clangd/compile_commands.json` を指定して精度の高い C++ 解析を実現
    3. **ビルド検証ステップ**: macOS runner で `make check` を実行し、コンパイルエラー・ワーニングを PR ごとにチェック
  - SonarQube Cloud 設定変更（管理画面）:
    - C file suffixes: `.c`（`.h` を削除）
    - C++ file suffixes: `.cpp`, `.h`（`.h` を追加）
    - `sonar.cfamily.analysisMode=compileCommands`

- **Click Drive エンベロープ制御（検討中）**
  - 目的: Click の Drive 量を Sub の Saturate と同様にエンベロープ LUT で時間変化させる
  - 現状の実装差異:
    - **Sub**: `dist_`（`float` 0〜1 正規化）で保持 → DSP 内で `× 24` して dB 変換 → `Saturator::process()`。エンベロープ LUT (`distLut_`) あり
    - **Click**: `driveDb_`（`float` 0〜24 dB 直値）で保持 → そのまま `Saturator::process()`。エンベロープ LUT なし
    - `clipType_`（`int` 0/1/2）は両者同一
  - 実装方針（Click 側を Sub 側の命名規則に合わせる）:
    1. `ClickEngine` に `EnvelopeLutManager driveLut_` を追加
    2. `driveDb_`（直値）→ `drive_`（0〜1 正規化）に変更し、DSP 内で `× 24` して dB 変換
    3. `setDriveDb(float db)` → `setDrive(float drive01)` に変更
    4. GUI 側: `driveSlider` の `onValueChange` で `v / 24.0f` を渡すよう修正
    5. `EnvelopeDatas` に `clickDrive` を追加し、`EnvelopeCurveEditor` に接続
  - 注意: Click の Drive は **UI ウィジェットは1個（共用）だが、値は `noiseState.drive` / `sampleState.drive` でモード別に独立保存・復元**されている。LUT を追加する場合も同様に Noise 用・Sample 用の2系統が必要（`ModeState` に `EnvelopeData` または LUT インデックスを追加してモード切り替え時に swap する形になる）

- **Direct チャンネル波形へのフィルター効果描画（検討中・方式未決定）**
  - 目的: Direct チャンネルの HPF/LPF パラメータをエンベロープカーブエディタの波形描画に反映し、フィルターの効果を視覚的にフィードバックする
  - 前提:
    - **音（DSP）は常にパラメータ通りに正確**。`renderPassthrough()` / `render()` で毎サンプル実フィルターを通している
    - 描画は近似。Amp エンベロープは純粋な振幅スケーリングなので高精度だが、フィルターは周波数特性の変形を伴うため入力のスペクトル分布に依存する
  - フィルター描画予測の性質比較:

    | | Amp | HPF/LPF |
    |---|---|---|
    | 変換の性質 | 振幅スケーリング（時間軸） | 周波数特性の変形（周波数軸） |
    | 予測方法 | `envelope(t)` を乗算 | 周波数ビンごとに振幅応答を掛ける（FFT → フィルター応答適用 → IFFT） |
    | 精度 | 高い（入力が定常なら正確） | 入力のスペクトル分布に依存 |

  - **方式 A: エンベロープなし — ノブ値のみで波形に反映（シンプル）**
    - フィルターパラメータは時間変化しないため、1回だけフィルター処理すればよい
    - Sample モード実装フロー（`refreshDirectProvider()` 拡張）:
      1. `thumbMin` / `thumbMax` を取得（すでにある）
      2. HPF/LPF のパラメータ（freq, Q, slope）で UI 用フィルターインスタンスを作成
      3. サムネイルデータをフィルターに通す → `filteredMin` / `filteredMax`
      4. `setDirectProvider()` に `filteredMin` / `filteredMax` を渡す
    - Passthrough モード: Timer コールバックで `realtimePixels_` を受け取る際に同様にフィルターを適用
    - **注意**: `thumbMin/thumbMax` は per-pixel min/max ペアであり、実際のオーディオサンプルではない。IIR フィルターに min/max を通すのは近似。より正確にするなら元サンプルデータをフィルターしてからサムネイルを再生成するが、可視化目的ならサムネイルへの直接適用でも概観は十分得られる
    - ノブ変更 → `refreshDirectProvider()` だけで更新完了（トリガーは `onValueChange` に追加するだけ）

  - **方式 B: エンベロープあり — フィルターパラメータを時間変化させる（将来拡張）**
    - フィルターの Freq / Q をエンベロープ LUT で時間制御する場合、描画も時間軸に沿った変化が必要
    - 実装方針: 各ピクセル（時間位置）ごとに、その時点のフィルターパラメータでの周波数応答 `|H(f)|` を計算し、入力波形に適用
    - 精度を上げるには FFT → `|H(f)|` 乗算 → IFFT が必要だが、計算コストが高い
    - 簡易近似: フィルターの `-3dB` ポイントと入力信号の主要周波数帯の関係から振幅スケーリングで代替（キック信号はスペクトルが予測しやすいため実用的）

  - 決定事項: **方式は後で決定する。** まずは方式 A（シンプル）で十分かを実装・評価し、必要に応じて方式 B に拡張

- **FilterResponseView（フィルター応答カーブ描画コンポーネント）**
  - 目的: Click / Direct パネルに HPF・LPF・BPF の周波数応答カーブを表示し、フィルター設定を視覚的にフィードバックする
  - 対象: **Click / Direct の Sample モード**（サムネイルデータが確定しているため正確に描画可能。Direct の Direct モード（リアルタイム入力）は上記の「Direct チャンネル波形へのフィルター効果描画」で扱う）
  - 対象チャンネル:
    - **Click**: BPF1（Noise モード専用）+ HPF + LPF（各最大4段カスケード）
    - **Direct**: HPF + LPF（各最大4段カスケード）
  - 実装方針: **`EnvelopeCurveEditor` とは別コンポーネント**（`FilterResponseView.h/.cpp`）
    - 理由: X軸が周波数（Hz, log）vs 時間（ms）で座標系が根本的に異なるため同居不適
    - SVF-TPT の解析的マグニチュード応答で描画（`g = tan(π·fc/fs)` → `|H(e^jω)|` 直接計算）
    - カスケード段数は `|H|^stages` で処理
  - API 設計:
    ```cpp
    FilterResponseView
    ├── setSampleRate(double sr)
    ├── setHpfParams(float freq, float q, int stages)
    ├── setLpfParams(float freq, float q, int stages)
    ├── setBpfParams(float freq, float q, int stages)  // Click 専用
    └── paint() → log-freq 軸で dB マグニチュード描画
    ```
  - 配置: Click パネル / Direct パネルにそれぞれインスタンスを配置
  - 見積もり: ~170行（`.h` + `.cpp`）。`EnvelopeCurveEditor.cpp` の行数は増えない

- **プリセット管理 + デフォルトプリセット**
  - 目的: 全パラメーター（エンベロープ含む）の初期値をハードコードではなく設定ファイルとして管理。ユーザーリセット・将来のプリセット追加に対応
  - 方式: **JUCE `BinaryData` 埋め込み**（外部ファイル依存なし、単一バイナリ配布を維持）
  - ディレクトリ構成:
    ```
    Resources/presets/
      default.xml       ← 出荷時デフォルト
      // 将来: fat_kick.xml, punchy.xml ...
    ```
  - CMakeLists.txt に追加:
    ```cmake
    juce_add_binary_data(BoomBabyBinaryData
        SOURCES Resources/presets/default.xml)
    ```
  - 実装手順:
    1. 全パラメーターを望ましい初期値にしてプラグインを起動
    2. `getStateInformation()` で XML をダンプして `Resources/presets/default.xml` として保存（フォーマット一致が保証される）
    3. `BinaryData` に追加
    4. `PluginProcessor` コンストラクタで `loadPresetFromXml(BinaryData::default_xml, ...)` を呼ぶ
    5. `setStateInformation()` に fallback: DAW データがなければ default を読む
  - 利点:
    - プリセット読み込みと DAW セッション復元が単一コードパスで統一
    - デフォルト値の変更がコード修正不要（XML 差し替えのみ）
    - ユーザーが「Default」を選ぶだけで全パラメーターをリセット可能
