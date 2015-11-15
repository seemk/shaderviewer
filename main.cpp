#include <stdio.h>
#include <GLFW/glfw3.h>
#include <bgfx/bgfxplatform.h>
#include <bgfx/bgfx.h>
#include "imgui/imgui.h"
#include "shaders.h"
#include <bx/timer.h>

GLFWwindow* GLOBAL_WINDOW = nullptr;

static const float screenQuad[] = {1.0f,  1.0f,  -1.0f, 1.0f,
                                   -1.0f, -1.0f, 1.0f,  -1.0f};

static const uint16_t quadIndices[] = {0, 1, 2, 2, 3, 0};

struct pos_vertex {
  float x, y;

  static void init() {
    ms_decl.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .end();
  }

  static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl pos_vertex::ms_decl;

struct mouse_state {
  int32_t x = 0;
  int32_t y = 0;
  int32_t scroll = 0;
  uint8_t button = 0;
};

struct stn_app {
  mouse_state mouse;
  int win_width = 1400;
  int win_height = 800;

  bgfx::ShaderHandle fs;
  bgfx::ShaderHandle vs;
  bgfx::ProgramHandle program;

  bgfx::UniformHandle u_resolution;
  bgfx::UniformHandle u_time;
  bgfx::UniformHandle u_mouse;
};

void glfw_key(GLFWwindow* window, int key, int, int action, int) {
  ImGuiIO& io = ImGui::GetIO();
  if (action == GLFW_PRESS) io.KeysDown[key] = true;
  if (action == GLFW_RELEASE) io.KeysDown[key] = false;

  io.KeyCtrl =
      io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
  io.KeyShift =
      io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
  io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];

  switch (key) {
    case GLFW_KEY_ESCAPE:
      if (action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
      break;
    default:
      break;
  }
}

void glfw_char_callback(GLFWwindow* window, unsigned int c) {
  ImGuiIO& io = ImGui::GetIO();
  if (c > 0 && c < 0x10000) io.AddInputCharacter((unsigned short)c);
}

void glfw_error(int error, const char* description) {
  printf("GLFW error %d: %s\n", error, description);
}

void glfw_resize(GLFWwindow* window, int width, int height) {
  stn_app* app = (stn_app*)glfwGetWindowUserPointer(window);
  app->win_width = width;
  app->win_height = height;

  bgfx::reset(width, height, BGFX_RESET_VSYNC);
}

void glfw_mousemove(GLFWwindow* window, double x, double y) {
  stn_app* app = (stn_app*)glfwGetWindowUserPointer(window);
  app->mouse.x = int32_t(x);
  app->mouse.y = int32_t(y);
}

void glfw_mousebtn(GLFWwindow* window, int button, int action, int) {
  stn_app* app = (stn_app*)glfwGetWindowUserPointer(window);

  switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      app->mouse.button ^= (-action ^ app->mouse.button) & IMGUI_MBUT_LEFT;
      break;
    default:
      break;
  }
}

int imgui_textedit_callback(ImGuiTextEditCallbackData* data) { 
  return 0;
}

int main(int argc, char** argv) {
  glfwSetErrorCallback(glfw_error);
  glfwInit();

  stn_app app;
  GLFWwindow* window =
      glfwCreateWindow(app.win_width, app.win_height, "STN", nullptr, nullptr);
  GLOBAL_WINDOW = window;

  glfwSetWindowUserPointer(window, &app);
  glfwSetKeyCallback(window, glfw_key);
  glfwSetCharCallback(window, glfw_char_callback);
  glfwSetWindowSizeCallback(window, glfw_resize);
  glfwSetCursorPosCallback(window, glfw_mousemove);
  glfwSetMouseButtonCallback(window, glfw_mousebtn);

  bgfx::glfwSetWindow(window);
  bgfx::init(bgfx::RendererType::OpenGL);
  bgfx::reset(app.win_width, app.win_height, BGFX_RESET_VSYNC);

  pos_vertex::init();

  app.u_time = bgfx::createUniform("uGlobalTime", bgfx::UniformType::Vec4);
  app.u_resolution = bgfx::createUniform("uResolution", bgfx::UniformType::Vec4);

  bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
      bgfx::makeRef(screenQuad, sizeof(screenQuad)), pos_vertex::ms_decl);
  bgfx::IndexBufferHandle ibh =
      bgfx::createIndexBuffer(bgfx::makeRef(quadIndices, sizeof(quadIndices)));
  
  app.vs = bgfx::createShader(bgfx::makeRef(vs_screen, sizeof(vs_screen)));
  app.fs = bgfx::createShader(bgfx::makeRef(testfs2, sizeof(testfs2)));

  app.program = bgfx::createProgram(app.vs, app.fs, true);

  imguiCreate();

  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6f6f6fff, 1.f,
                     0);

  const size_t texbuf_size = 8092;
  char texbuf[texbuf_size];
  memset(texbuf, 0, texbuf_size);
  ImGuiIO& io = ImGui::GetIO();
  io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
  io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
  io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
  io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
  io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
  io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
  io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
  io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
  io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
  io.GetClipboardTextFn =
      []() { return glfwGetClipboardString(GLOBAL_WINDOW); };
  io.SetClipboardTextFn =
      [](const char* text) { glfwSetClipboardString(GLOBAL_WINDOW, text); };

  
  int64_t cur_time = bx::getHPCounter();
  int64_t prev_time = cur_time;
  double total_time = 0.0;
  while (!glfwWindowShouldClose(window)) {
    prev_time = cur_time;
    cur_time = bx::getHPCounter();
    const int64_t frame_time = cur_time - prev_time;
    const double freq = double(bx::getHPFrequency());
    const double to_ms = 1000.0 / freq;
    const double dt_ms = double(frame_time) * to_ms;
    const double dt = dt_ms / 1000.0;
    total_time += dt;

    glfwPollEvents();

    bgfx::setViewRect(0, 0, 0, app.win_width, app.win_height);

    bgfx::touch(0);

    imguiBeginFrame(app.mouse.x, app.mouse.y, app.mouse.button, 0,
                    app.win_width, app.win_height);

    ImGui::InputTextMultiline("", texbuf, texbuf_size,
                              ImVec2(500, app.win_height));
    imguiEndFrame();

    float mouse_unif[4] = { float(app.mouse.x), float(app.mouse.y), 0.0f, 0.0f };
    if (app.mouse.button) {
      mouse_unif[2] = float(app.mouse.x);
      mouse_unif[3] = float(app.mouse.y);
    }
    bgfx::setUniform(app.u_mouse, mouse_unif);
    const float reso_unif[4] = { float(app.win_width), float(app.win_height), 0.f, 0.f };
    bgfx::setUniform(app.u_resolution, reso_unif);
    const float time_unif[4] = { float(total_time), 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(app.u_time, time_unif);
    bgfx::setVertexBuffer(vbh);
    bgfx::setIndexBuffer(ibh);
    bgfx::submit(0, app.program);

    bgfx::frame();
  }

  imguiDestroy();
  bgfx::shutdown();
  glfwTerminate();

  return 0;
}