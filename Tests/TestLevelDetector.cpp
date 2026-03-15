#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/LevelDetector.h"

using namespace Catch::Matchers;

// TODO: LevelDetector の各テストを実装する
//
// テスト対象:
//   - ピーク検出の正確性: 既知の振幅を入力して期待値と一致すること
//   - リリース特性: ピーク後に正しい速度で減衰すること
//   - ロックフリー操作の安全性（std::atomic ロード/ストア）

TEST_CASE("LevelDetector: placeholder", "[level_detector]") {
  SUCCEED("TODO: implement");
}
