#include "shadergen.h"
#include <stdio.h>
#include <string>
#include <string.h>
#include <cctype>
#include "glsl/glsl_optimizer.h"

std::vector<uint8_t> gen_fragshader(const char* code, size_t len) {

  std::vector<uint8_t> bgfx_shader = {
    0x46, 0x53, 0x48, 0x04, // FSH4
    0x00, 0x00, 0x00, 0x00, // iohash
    0x00, 0x00 // count
  };

  std::vector<uint8_t> shader;
  const std::string prefix = R"(
    uniform vec4 uResolution;
    uniform vec4 uGlobalTime;
    uniform vec4 iMouse;
    uniform sampler2D iChannel0;
    uniform sampler2D iChannel1;

    vec3 iResolution;
    float iGlobalTime;
  )";

  const std::string shader_main = R"(
    void main() {
      vec4 color = vec4(1.0);
      iResolution = uResolution.xyz;
      iGlobalTime = uGlobalTime.x;
      mainImage(color, gl_FragCoord.xy);
      gl_FragColor = color;
    }
  )";

  shader.insert(shader.end(), prefix.begin(), prefix.end());

  for (size_t i = 0; i < len; i++) {
    char c = code[i];
    if (c == 0) break;

    shader.push_back(c);
  }

  shader.insert(shader.end(), shader_main.begin(), shader_main.end());
  shader.push_back(0x00);

#if 1
  const size_t shader_size = shader.size();
  const uint8_t* size_begin = (const uint8_t*)&shader_size;
  bgfx_shader.insert(bgfx_shader.end(), size_begin, size_begin + sizeof(uint32_t));
  bgfx_shader.insert(bgfx_shader.end(), shader.begin(), shader.end());
#else
  const glslopt_shader_type type = kGlslOptShaderFragment;
  const glslopt_target target = kGlslTargetOpenGLES20;
  glslopt_ctx* opt_ctx = glslopt_initialize(target);
  glslopt_shader* opt_shader = glslopt_optimize(opt_ctx, type, (const char*)shader.data(), 0);
  if (!glslopt_get_status(opt_shader)) {
    const char* log = glslopt_get_log(opt_shader);
    printf("failed to optimize shader:\n%s\n", log);
    glslopt_shader_delete(opt_shader);
    glslopt_cleanup(opt_ctx);
    return std::vector<uint8_t>();
  }

  const char* output = glslopt_get_output(opt_shader);
  const size_t output_len = strlen(output);

  const uint8_t* output_begin = (const uint8_t*)output;
  const uint8_t* output_end = output_begin + output_len;

  const uint8_t* size_begin = (const uint8_t*)&output_len;
  bgfx_shader.insert(bgfx_shader.end(), size_begin, size_begin + sizeof(uint32_t));

  bgfx_shader.insert(bgfx_shader.end(), output_begin, output_end);
  bgfx_shader.push_back(0x00);
  glslopt_shader_delete(opt_shader);
  glslopt_cleanup(opt_ctx);
#endif


  printf("shader size: %lu\n", bgfx_shader.size());

  return bgfx_shader;
}
