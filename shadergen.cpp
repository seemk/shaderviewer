#include "shadergen.h"
#include <stdio.h>
#include <string>
#include <string.h>
#include <cctype>

std::vector<uint8_t> gen_fragshader(const char* code, size_t len) {

  std::vector<uint8_t> shader_code = {
    0x46, 0x53, 0x48, 0x04, // FSH4
    0x00, 0x00, 0x00, 0x00, // iohash
    0x00, 0x00 // count
  };

  const std::string prefix = R"(uniform vec4 uResolution;
    uniform vec4 uGlobalTime;
    uniform vec4 iMouse;

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

  uint32_t shader_size = prefix.length() + len + shader_main.length();
  const uint8_t* size_begin = (const uint8_t*)&shader_size;
  shader_code.insert(shader_code.end(), size_begin, size_begin + sizeof(uint32_t));
  shader_code.insert(shader_code.end(), prefix.begin(), prefix.end());

  const uint8_t* code_begin = (const uint8_t*)code;
  const uint8_t* code_end = code_begin + len;

  shader_code.insert(shader_code.end(), code_begin, code_end);
  shader_code.insert(shader_code.end(), shader_main.begin(), shader_main.end());
  shader_code.push_back(0x00);

  for (uint8_t c : shader_code) {
    printf("0x%02x ", c);
  }

  printf("\nText shader size: %lu\n", shader_size);
  printf("\nShader size: %lu\n", shader_code.size());

  return shader_code;
}
