#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/DirectEngine.h"

using namespace Catch::Matchers;

// TODO: DirectEngine の各テストを実装する
//
// テスト対象:
//   - renderPassthrough: amp==0 または s==0
//   で早期リターンし出力がゼロになること
//     （Tube バイアスリーク回帰テスト: アクティブでないのに DC が乗らないこと）
//   - Sample モード: triggerNote 後に正常に再生が開始されること

TEST_CASE("DirectEngine: placeholder", "[direct_engine]") {
  SUCCEED("TODO: implement");
}
