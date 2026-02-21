#pragma once

#include <juce_opengl/juce_opengl.h>
#include <mutex>
#include <vector>

/// OpenGL GPU 描画による波形ディスプレイ
/// - UIスレッドから updateWaveform() でデータを受け取り
/// - OpenGLスレッドの renderOpenGL() で GL_LINE_STRIP 描画
class WaveformDisplay : public juce::Component, public juce::OpenGLRenderer {
public:
  WaveformDisplay();
  ~WaveformDisplay() override;

  /// UIスレッド（60Hz Timer）から呼び出し。波形サンプルを描画バッファにコピー。
  void updateWaveform(const float *data, int numSamples);

  // ── juce::OpenGLRenderer ──
  void newOpenGLContextCreated() override;
  void renderOpenGL() override;
  void openGLContextClosing() override;

private:
  static constexpr int maxDisplaySamples = 1024;

  juce::OpenGLContext glContext;

  // UIスレッド → GLスレッド 間のデータ受け渡し
  std::mutex dataMutex;
  std::vector<float> displayBuffer; // updateWaveform で書き込み
  bool dataReady = false;

  // GL リソース
  GLuint vbo = 0;
  GLuint vao = 0;
  std::unique_ptr<juce::OpenGLShaderProgram> shader;

  // renderOpenGL 内で使うローカルバッファ（mutex 外でGLアップロード）
  std::vector<float> renderBuffer;

  void buildShader();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
