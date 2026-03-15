#include <catch2/catch_test_macros.hpp>

#include "DSP/ClickEngine.h"

// TODO: ClickEngine の各テストを実装する
//
// テスト対象:
//   - triggerNote() 後にフィルター状態がリセットされること
//   - 無音入力（Noise モード）→ エンジン非アクティブ時は出力がゼロになること
//   - BPF カスケードが正しくリセットされること

TEST_CASE("ClickEngine: placeholder", "[click_engine]") {
  SUCCEED("TODO: implement");
}
