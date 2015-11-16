cmake_minimum_required(VERSION 3.0)
project(shaderviewer)


add_subdirectory("third_party/glfw-3.1.1")
add_subdirectory("third_party/glsl_optimizer")

include_directories(
  "third_party/glfw-3.1.1/include"
  "third_party/glsl_optimizer/src"
  "third_party/bx/include"
  "third_party/bgfx/include"
  "stb"
)

if (WIN32)
  include_directories("third_party/bx/include/compat/msvc")
else()
  add_definitions(-std=c++11 -Wall -Wextra)
endif()

add_subdirectory("third_party/bgfx")

set(IMGUI_SOURCES
  imgui/ocornut-imgui/imgui.cpp
  imgui/ocornut-imgui/imgui_draw.cpp
  imgui/ocornut-imgui/imgui_wm.cpp
  imgui/imgui.cpp
  imgui/ocornut_imgui.cpp
)

set(NANOVG_SOURCES
  nanovg/nanovg.cpp
  nanovg/nanovg_bgfx.cpp
)

if (WIN32)
else()
  add_definitions(-std=c++11 -Wall -Wextra)
endif()

add_executable(stn
  main.cpp
  shaders.h
  shadergen.h
  shadergen.cpp
  ${NANOVG_SOURCES}
  ${IMGUI_SOURCES}
)

target_link_libraries(
  stn
  bgfx
  glsl_optimizer
  glfw
  ${GLFW_LIBRARIES}
)

add_custom_command(TARGET stn POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:stn>)
