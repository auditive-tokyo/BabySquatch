#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/EnvelopeLutManager.h"

using namespace Catch::Matchers;

// TODO: EnvelopeLutManager の各テストを実装する
//
// テスト対象:
//   - computeAmp の値域が [0, 1] に収まること
//   - duration 外のサンプルは 0 を返すこと
//   - LUT
//   切替後も前バッファへのアクセスが安全なこと（ロックフリー二重バッファ）

TEST_CASE("EnvelopeLutManager: placeholder", "[envelope_lut]") {
  SUCCEED("TODO: implement");
}
