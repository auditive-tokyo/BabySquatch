# BabySquatch Project

## プロジェクト概要

BabySquatchは、Boz LabのSasquatchから自分が使う機能だけを集約したキックエンハンスプラグインです。

## プラグイン構成

BabySquatchは3つのモジュールで構成されています：

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
- C++20
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

- **Auto Trigger UI 拡張（Threshold / Hold）**
  - 現状: 閾値 -24 dBFS・Hold 50ms は固定値。以下の UI を追加して調整可能にする
  - **配置方針（Direct Insert FX と同時実装）**:
    - **Hold** → `mode:` ドロップダウンの右側にスライダーとして配置（Direct パネル上部）
    - **Threshold** → パススルーモード時に Pitch ノブの位置へ差し替え（Sample モード時は Pitch ノブが復帰）
  - **Hold — ms 直値スライダー（第1フェーズ）**
    - ウィジェット: 横向きスライダー（`mode:` ドロップダウン右の余白に収める）
    - 範囲: 20 〜 500 ms、デフォルト 50 ms
    - `TransientDetector::setHoldMs(float ms)` を追加して接続
    - 将来的に BPM × Note Division 方式への拡張も可（第2フェーズ以降）
  - **Threshold — ノブ（Pitch 位置に配置）**
    - パススルーモード時のみ表示（`isDirectPassthrough()` で切り替え）
    - 範囲: -60 〜 0 dBFS（デフォルト -24 dBFS）
    - 差分検出（Fast − Slow Envelope）の **onset 差分値** に対する閾値（絶対信号レベルとは別物）
      - 低い値（感度高）: 微小な立ち上がりでもトリガー → ゴーストノートや残響に反応しやすい
      - 高い値（感度低）: 明確なアタックのみトリガー → 本打ちキックだけを確実に拾う
    - `TransientDetector::setThresholdDb(float db)` を追加して接続
    - 入力波形リアルタイム表示と組み合わせ、Threshold 水平線を波形上に重ねて表示

- **Direct パラメーターをパススルー入力に適用（Direct Insert FX）**
  - **現状の問題**: `directSampleMode_ = false`（パススルー）のとき `DirectEngine::render()` は `active_ = false` で早期リターンし、入力信号はすべてのパラメーターをバイパスしてそのまま通過する。Channel Fader も無効。適用されるのは Master Gain のみ。
  - **目的**: パススルーモード時に入力信号を DirectEngine の各パラメーターで整形できるようにする。キックのHighPass・Drive・Amp/Decay 形状を入力に乗せることで Sub/Click との音色統一感を高める。

  **設計方針（Opus レビュー後確定版）**:
  - Sampleモードとパススルーモードは排他（`directSampleMode_` フラグで切り替え）→ フィルター（`hpfs_`/`lpfs_`）・再生カウンター（`active_`/`noteTimeSamples_`）は**共用可能**。専用配列の新設は不要でありCPU/メモリ双方で無駄になるため採用しない
  - `render()` のシグネチャは変更せず、パススルー専用メソッド `renderPassthrough()` を別途新設して責務を分離する
  - パススルー時のモノミックス→`addSample()`方式を採用（キックは実用上モノで問題なし）。ステレオ in-place 処理は将来的に検討

  **実装ステップ（順序厳守）**:

  1. **`DirectEngine` にパススルーモードフラグを追加**
     - `std::atomic<bool> passthroughMode_{false}` をメンバーに追加
     - `void setPassthroughMode(bool b) noexcept { passthroughMode_.store(b); }` を public setter として追加
     - `PluginProcessor` がモード切り替え時に呼ぶ（`directSampleMode_` セット箇所と同タイミング）

  2. **`triggerNote()` のガード条件修正**
     - 現状: `if (!sampler_.isLoaded()) return;` → パススルー時はサンプルが存在しないためトリガーされない
     - 修正: パススルーモード時はサンプルチェックをスキップしてトリガーする
       ```cpp
       void DirectEngine::triggerNote() {
           if (!passthroughMode_.load() && !sampler_.isLoaded()) return;
           // パススルー時はサンプルのresetPlayheadは不要（スキップ）
           if (!passthroughMode_.load()) sampler_.resetPlayhead();
           noteTimeSamples_ = 0.0f;
           for (auto& f : hpfs_) f.reset();  // フィルター状態をリセット（共用）
           for (auto& f : lpfs_) f.reset();
           active_.store(true);
       }
       ```
     - `hpfs_`/`lpfs_` のリセットをここで行うことで、モード切り替え直後のフィルター状態汚染を防ぐ

  3. **`renderPassthrough()` メソッドを新設**
     - シグネチャ: `void renderPassthrough(juce::AudioBuffer<float>& buffer, std::span<const float> inputMono, int numSamples, double sampleRate)`
     - 既存の `hpfs_`/`lpfs_`/`active_`/`noteTimeSamples_` を**そのまま共用**（Sample モードと同一変数、排他なので干渉しない）
     - 既存の `computeSampleAmp()` / `prepareFilters()` / `computeMaxTimeSamples()` もそのまま流用（ただし `computeMaxTimeSamples` は LUT 長のみ使用）
     - ループ処理:
       ```cpp
       for (int i = 0; i < numSamples; ++i) {
           float amp = 1.0f;
           if (active_.load()) {
               if (noteTimeSamples_ >= maxTimeSamples) { active_.store(false); } // Decay終了
               else { amp = computeSampleAmp(noteTimeSamples_ * 1000.0f / sr); }
               noteTimeSamples_ += 1.0f;
           } else { amp = 0.0f; }  // Decay終了後はミュート（Amp Envelope ON 時）
           float s = inputMono[i];
           // Drive（プリフィルター）
           s = Saturator::process(s, driveDb_.load(), clipType_.load());
           // HPF / LPF
           for (int fi = 0; fs.doHpf && fi < fs.hpfStg; ++fi)
               s = hpfs_[fi].processSample(0, s);
           for (int fi = 0; fs.doLpf && fi < fs.lpfStg; ++fi)
               s = lpfs_[fi].processSample(0, s);
           // 共振ピーク整形（既存サンプル再生と同一ロジック）
           if (fs.doHpf || fs.doLpf) s = Saturator::process(s, 0.0f, clipType_.load());
           s *= gain * amp;
           for (int ch = 0; ch < numCh; ++ch) buffer.addSample(ch, i, s);
       }
       ```
     - `scratchBuffer_[i] = s` への書き込みも忘れずに行う（Channel Fader のレベル計測 `scratchData()` が参照するため）

  4. **`processBlock()` のバッファクリア論理修正（重要）**
     - **現状のバグ**: パススルー（`passes.direct=true, directSampleMode_=false`）時はクリア条件 `!passes.direct || directSampleMode_.load()` が `false` になり `buffer.clear()` が呼ばれない。入力信号がバッファに残ったまま `renderPassthrough()` が `addSample()` すると**未処理ステレオ入力 + 処理済みモノ信号**が合算される二重信号バグになる
     - **修正**: パススルーモードでも `buffer.clear()` を行い、`renderPassthrough()` で処理済み信号のみを加算する
       ```cpp
       // 修正前: if (!passes.direct || directSampleMode_.load()) buffer.clear();
       // 修正後:
       if (!passes.direct) {
           buffer.clear();                    // Direct OFF: 全消去
       } else if (directSampleMode_.load()) {
           buffer.clear();                    // Sample モード: クリア→サンプル再生で加算
       } else {
           buffer.clear();                    // パススルーモード: クリア→処理済み入力を加算
       }
       // → 全 case で buffer.clear() → 以下の1行で統一可:
       if (!passes.direct || true) buffer.clear(); // または単純に buffer.clear() を常時呼ぶ
       ```
     - 上記を整理すると: `buffer.clear()` を常に呼ぶ（Direct OFF 時は後続の render も呼ばれないため問題なし）

  5. **`processBlock()` での呼び分け**
     - パススルーモード時は `render()` の代わりに `renderPassthrough()` を呼ぶよう分岐:
       ```cpp
       if (!directSampleMode_.load() && passes.direct) {
           directEngine_.renderPassthrough(buffer,
               std::span<const float>(monoMixBuffer_.data(), numSamples),
               numSamples, sr);
       } else {
           directEngine_.render(buffer, numSamples, passes.direct, sr);
       }
       ```
     - `monoMixBuffer_` はパススルーモード時にのみ有効（既存コードで `!directSampleMode_.load()` ガードの中で生成済み）

  6. **Pitch ノブ → Threshold ノブへの差し替え（パススルーモード時）**
     - リアルタイム入力へのピッチシフトはアルゴリズム的に不可（位相ボコーダー等は遅延＋高CPU）のため、Pitch ノブは削除ではなく**非表示**にして同位置に Threshold ノブを表示
     - `DirectParams.cpp` にて: `pitchSlider_.setVisible(!isPassthrough)` / `thresholdKnob_.setVisible(isPassthrough)` で切り替え
     - `PluginEditor::timerCallback()` 内で `isDirectPassthrough()` の変化を検出して `directParams.refreshPassthroughUI()` 等を呼ぶ
  7. **Hold スライダーの追加（mode ドロップダウン右）**
     - `DirectParams.cpp` にて: `mode:` ドロップダウンの右側に横向きスライダーを追加
     - 範囲: 20 〜 500 ms / デフォルト: 50 ms
     - `onValueChange` → `processor.transientDetector_.setHoldMs(v)` で接続
     - Auto Trigger がオフの場合はグレーアウト（`holdSlider_.setEnabled(autoEnabled)`）

  **パラメーター適用可否まとめ**:
  | パラメーター | 適用 | 備考 |
  |---|---|---|
  | Amp (Gain) | ✅ | `gainDb_` 共用 |
  | Decay LUT (Amp Envelope) | ✅ | `active_`/`noteTimeSamples_` を共用。`triggerNote()` 連動 |
  | Drive + Clip Type | ✅ | `monoMixBuffer_` を `Saturator::process()` に通す |
  | HP Freq / Q / Slope | ✅ | `hpfs_` を共用（フィルター二重化なし） |
  | LP Freq / Q / Slope | ✅ | `lpfs_` を共用（フィルター二重化なし） |
  | Pitch (semitones) | ❌ | パススルー時は非表示→同位置に Threshold ノブを表示 |
  | Channel Fader | ✅ | `scratchBuffer_` への書き込みにより `scratchData()` 経由でレベル計測も正常動作 |

- **レイテンシー補正（Delay Compensation）**
  - 背景: BabySquatch は Direct（入力パススルー）と Sub/Click（生成信号）を混合するため、どちらかに処理遅延が生じると内部でタイミングズレが発生する
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
    juce_add_binary_data(BabySquatchBinaryData
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
