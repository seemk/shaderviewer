#include <stdio.h>
#include <GLFW/glfw3.h>
#include <bgfx/bgfxplatform.h>
#include <bgfx/bgfx.h>
#include "imgui/imgui.h"
#include "shaders.h"
#include <bx/timer.h>
#include <bx/debug.h>
#include <bx/string.h>
#include "shadergen.h"
#include <vector>
#include <stb/stb_image.c>
#include <cstring>

GLFWwindow* GLOBAL_WINDOW = nullptr;

union bgfx_ig_texture {
  ImTextureID ptr;
  struct {
    uint16_t flags;
    bgfx::TextureHandle handle;
  } bgfx_texture;
};

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

struct shader_channel {
  int texture_id = -1;
  bgfx::UniformHandle uniform;
};

struct stn_app {
  stn_app() : shader_buffer(shader_buf_size, 0) {
    std::strncpy(shader_buffer.data(), default_fs, shader_buf_size);
  }

  mouse_state mouse;
  int win_width = 1000;
  int win_height = 600;

  std::vector<bgfx::TextureHandle> textures;

  bgfx::ShaderHandle vs;
  bgfx::ProgramHandle program;

  bgfx::UniformHandle u_resolution;
  bgfx::UniformHandle u_time;
  bgfx::UniformHandle u_mouse;

  const size_t shader_buf_size = 100000;
  std::vector<char> shader_buffer;

  std::vector<uint8_t> shader_code;

  std::vector<shader_channel> channels;
};

bgfx::TextureHandle load_texture(const char* file) {
  bgfx::TextureHandle texture;
  int w, h, n;
  unsigned char* texture_data = stbi_load(file, &w, &h, &n, 4);
  if (texture_data) {
    texture = bgfx::createTexture2D(w, h, 1, bgfx::TextureFormat::RGBA8, 0,
                                    bgfx::copy(texture_data, w * h * n));
  }

  return texture;
};

static const char* const texture_files[] = {"nochan.png", "tex00.jpg",
                                            "tex15.png", "tex16.png"};

struct bgfxCallback : public bgfx::CallbackI {
  virtual ~bgfxCallback() {}

  virtual void traceVargs(const char* _filePath, uint16_t _line,
                          const char* _format, va_list _argList) override {
    char temp[2048];
    char* out = temp;
    int32_t len =
        bx::snprintf(out, sizeof(temp), "%s (%d): ", _filePath, _line);
    int32_t total =
        len + bx::vsnprintf(out + len, sizeof(temp) - len, _format, _argList);
    if ((int32_t)sizeof(temp) < total) {
      out = (char*)alloca(total + 1);
      memcpy(out, temp, len);
      bx::vsnprintf(out + len, total - len, _format, _argList);
    }
    out[total] = '\0';
    bx::debugOutput(out);
  }

  virtual void fatal(bgfx::Fatal::Enum _code, const char* _str) override {
    if (bgfx::Fatal::DebugCheck == _code) {
      bx::debugBreak();
    } else if (bgfx::Fatal::InvalidShader == _code) {
      printf("invalid shader\n");
      printf("%s\n", _str);
    } else {
      BX_TRACE("0x%08x: %s", _code, _str);
      BX_UNUSED(_code, _str);
      abort();
    }
  }

  virtual uint32_t cacheReadSize(uint64_t) override { return 0; }

  virtual bool cacheRead(uint64_t, void*, uint32_t) override { return false; }

  virtual void cacheWrite(uint64_t, const void*, uint32_t) override {}

  virtual void screenShot(const char* _filePath, uint32_t _width,
                          uint32_t _height, uint32_t _pitch, const void* _data,
                          uint32_t _size, bool _yflip) override {
    BX_UNUSED(_filePath, _width, _height, _pitch, _data, _size, _yflip);
  }

  virtual void captureBegin(uint32_t, uint32_t, uint32_t,
                            bgfx::TextureFormat::Enum, bool) override {}

  virtual void captureEnd() override {}

  virtual void captureFrame(const void*, uint32_t) override {}
};

void glfw_key(GLFWwindow* window, int key, int, int action, int) {
  stn_app* app = (stn_app*)glfwGetWindowUserPointer(window);

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
    case GLFW_KEY_F5:
      if (action == GLFW_RELEASE) {
        std::vector<uint8_t> shader_code = gen_fragshader(
            app->shader_buffer.data(), app->shader_buffer.size());

        if (!shader_code.empty()) {
          app->shader_code = std::move(shader_code);
          bgfx::ShaderHandle fs = bgfx::createShader(
              bgfx::makeRef(app->shader_code.data(), app->shader_code.size()));
          bgfx::ProgramHandle np = bgfx::createProgram(app->vs, fs);
          bgfx::destroyShader(fs);
          bgfx::destroyProgram(app->program);
          app->program = np;
        }
      }
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

void glfw_scroll(GLFWwindow* window, double x, double y) {
  stn_app* app = (stn_app*)glfwGetWindowUserPointer(window);
  app->mouse.scroll += int32_t(y);
}

int imgui_textedit_callback(ImGuiTextEditCallbackData* data) { return 0; }

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
  glfwSetScrollCallback(window, glfw_scroll);

  bgfxCallback cb;

  bgfx::glfwSetWindow(window);
  bgfx::init(bgfx::RendererType::OpenGL, BGFX_PCI_ID_NONE, 0, &cb);
  bgfx::setDebug(BGFX_DEBUG_TEXT);
  bgfx::reset(app.win_width, app.win_height, BGFX_RESET_VSYNC);

  pos_vertex::init();

  app.u_time = bgfx::createUniform("uGlobalTime", bgfx::UniformType::Vec4);
  app.u_resolution =
      bgfx::createUniform("uResolution", bgfx::UniformType::Vec4);
  app.u_mouse = bgfx::createUniform("iMouse", bgfx::UniformType::Vec4);

  bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
      bgfx::makeRef(screenQuad, sizeof(screenQuad)), pos_vertex::ms_decl);
  bgfx::IndexBufferHandle ibh =
      bgfx::createIndexBuffer(bgfx::makeRef(quadIndices, sizeof(quadIndices)));

  app.vs = bgfx::createShader(bgfx::makeRef(vs_screen, sizeof(vs_screen)));

  std::vector<uint8_t> default_shader = gen_fragshader(default_fs, strlen(default_fs));
  bgfx::ShaderHandle fs =
      bgfx::createShader(bgfx::makeRef(default_shader.data(), default_shader.size()));

  app.program = bgfx::createProgram(app.vs, fs);
  bgfx::destroyShader(fs);

  for (const char* texture_file : texture_files) {
    bgfx::TextureHandle texture = load_texture(texture_file);
    if (bgfx::isValid(texture)) {
      app.textures.push_back(texture);
    }
  };

  for (int i = 0; i < 4; i++) {
    shader_channel c;
    c.texture_id = 0;
    std::string name = "iChannel" + std::to_string(i);
    c.uniform = bgfx::createUniform(name.c_str(), bgfx::UniformType::Int1);
    app.channels.push_back(c);
  }

  imguiCreate();

  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6f6f6fff, 1.f,
                     0);

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

  bool opened = true;
  int selecting_channel = -1;
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

    ImGuiWindowFlags window_flags = 0;
    imguiBeginFrame(app.mouse.x, app.mouse.y, app.mouse.button,
                    app.mouse.scroll, app.win_width, app.win_height);

    ImGui::Begin("options", &opened);

    ImGui::BeginChild("code", ImVec2(500, 500));
    ImGui::InputTextMultiline("", app.shader_buffer.data(),
                              app.shader_buffer.size(),
                              ImVec2(500, app.win_height));
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("textures", ImVec2(200, 500));
  
    for (size_t i = 0; i < app.channels.size(); i++) {
      const shader_channel* chan = &app.channels[i];
      bgfx_ig_texture img;
      if (chan->texture_id >= 0) {
        img.bgfx_texture = {0, app.textures[chan->texture_id]};
      } else {
        img.bgfx_texture = {0, {bgfx::invalidHandle}};
      }
      ImGui::PushID(i);
      if (ImGui::ImageButton(img.ptr, ImVec2(148, 96), ImVec2(0, 0),
                             ImVec2(1, 1), 1, ImVec4(0, 0, 0, 255),
                             ImVec4(1, 1, 1, 1))) {
        selecting_channel = int(i);
      }
      ImGui::PopID();
    }

    if (selecting_channel > -1) {
      const char* title = "Select texture";
      ImGui::OpenPopup(title);
      if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

        for (size_t i = 0; i < app.textures.size(); i++) {
          bgfx_ig_texture img;
          img.bgfx_texture = {0, app.textures[i]};
          if (ImGui::ImageButton(img.ptr, ImVec2(148, 96))) {
            app.channels[selecting_channel].texture_id = i;
            selecting_channel = -1;
            ImGui::CloseCurrentPopup();
          }

          if (i % 3 < 2) {
            ImGui::SameLine();
          }
        }

        ImGui::EndPopup();
      }
    }


    ImGui::EndChild();
    ImGui::End();
    imguiEndFrame();

    for (size_t i = 0; i < app.channels.size(); i++) {
      const shader_channel* ch = &app.channels[i];
      if (ch->texture_id > 0) {
        bgfx::TextureHandle texture = app.textures[ch->texture_id];
        bgfx::setTexture(i, ch->uniform, texture);
      }
    }

    float mouse_unif[4] = {float(app.mouse.x), float(app.mouse.y), 0.0f, 0.0f};
    if (app.mouse.button) {
      mouse_unif[2] = float(app.mouse.x);
      mouse_unif[3] = float(app.mouse.y);
    }
    bgfx::setUniform(app.u_mouse, mouse_unif);
    const float reso_unif[4] = {float(app.win_width), float(app.win_height),
                                0.f, 0.f};
    bgfx::setUniform(app.u_resolution, reso_unif);
    const float time_unif[4] = {float(total_time), 0.0f, 0.0f, 0.0f};
    bgfx::setUniform(app.u_time, time_unif);
    bgfx::setVertexBuffer(vbh);
    bgfx::setIndexBuffer(ibh);
    bgfx::submit(0, app.program);

    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 0, 0x04f, "frame: %4.2f ms", dt_ms);

    bgfx::frame();
  }

  imguiDestroy();
  bgfx::shutdown();
  glfwTerminate();

  return 0;
}
