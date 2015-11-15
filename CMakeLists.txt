cmake_minimum_required(VERSION 3.0)
project(shaderviewer)


add_subdirectory("third_party/glfw-3.1.1")

include_directories(
  "third_party/glfw-3.1.1/include"
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
  ${NANOVG_SOURCES}
  ${IMGUI_SOURCES}
)

target_link_libraries(
  stn
  bgfx
  glfw
  ${GLFW_LIBRARIES}
)
