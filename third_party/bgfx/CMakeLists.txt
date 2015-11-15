project(bgfx)

include_directories(
  "third_party/khronos"
  "include"
)

add_definitions(
  -DBGFX_CONFIG_MULTITHREADED=0
  -DBGFX_CONFIG_RENDERER_OPENGL=1
)

add_library(bgfx src/amalgamated.cpp)

