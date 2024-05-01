#include <stdio.h>
#include <cstddef>

#ifdef __EMSCRIPTEN__
#include <SDL_opengles2.h>
#else
#include <glew.h>
// glew has to be included first
#include <SDL_opengl.h>
#include <glu.h>
#endif

#include <SDL.h>

#include <SDL_keycode.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten_mainloop_stub.h>
#endif

#include <gl_util.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#define TIMESTEP 0.0083333333333

int deInitSDL(SDL_Window* window, SDL_GLContext* context);
int deInitImgui();
void initJoysticks();

GLuint passthrough_shader;
bool g_vsync = 0;
// placeholder graphics
JGL::Font g_font;

bool DebugInput(SDL_Event* event, SDL_Window* window);

int main(int argc, char** argv) {
  float slowdown = 1;
  float acc = 0;
  Uint64 g_time = 0;
  float delta_time;
  float average_frame_time = -1;
  float highest_frame_time = 0;
  int w, h;
  Uint64 tick = 0;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }
  printf("started...\n");

#ifdef __EMSCRIPTEN__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
  // SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,1);
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window* window = SDL_CreateWindow("jumphysics sandbox", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, 640, 400, window_flags);
  printf("SDL_CreateWindow error: %s\n", SDL_GetError());
  SDL_GLContext context = SDL_GL_CreateContext(window);
  printf("SDL_GL_CreateContext error: %s\n", SDL_GetError());

  SDL_GL_MakeCurrent(window, context);
  printf("SDL_GL_MakeCurrent error: %s\n", SDL_GetError());
  SDL_GL_SetSwapInterval(g_vsync);
  printf("SDL_GL_SetSwapInterval error: %s\n", SDL_GetError());

#ifndef __EMSCRIPTEN__
  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();
  if (glewError != GLEW_OK) {
    printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
  }
#endif

  initJoysticks();

  printf("vendor: %s\n renderer: %s\n version %s\n glsl version %s\n", glGetString(GL_VENDOR),
         glGetString(GL_RENDERER), glGetString(GL_VERSION),
         glGetString(GL_SHADING_LANGUAGE_VERSION));

  JGL::init();
  // placeholder graphics
  passthrough_shader =
      JGL::loadShaderProgram("../assets/passthrough.vert", "../assets/passthrough.frag");
  JGL::loadFont("../assets/DroidSans.ttf", &g_font);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  printf("started...\n");

  // main loop
  bool done = false;
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (!done)
#endif
  {
    SDL_GL_GetDrawableSize(window, &w, &h);

    // Get timestep
    {
      // (we don't use SDL_GetTicks() because it is using millisecond resolution)
      static Uint64 frequency = SDL_GetPerformanceFrequency();  // TODO: why did i make this static
      Uint64 current_time = SDL_GetPerformanceCounter();
      // SDL_GetPerformanceCounter() not returning a monotonically increasing value in Emscripten sometimes
      if (current_time <= g_time) {
        current_time = g_time + 1;
      }
      if (g_time > 0) {
        delta_time = (float)((double)(current_time - g_time) / frequency);
      } else {
        delta_time = (float)(1.0f / 60.0f);
      }
      g_time = current_time;
    }

    // frame time stats
    if (delta_time > highest_frame_time) {
      highest_frame_time = delta_time;
    }
    if (average_frame_time > 0) {
      average_frame_time = average_frame_time * 0.95 + delta_time * 0.05;
    } else {  // get initial value
      average_frame_time = delta_time;
    }

    acc += delta_time;
    while (acc > TIMESTEP * slowdown) {
      // process input events every tick
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        done = DebugInput(&event, window);
      }

      acc -= TIMESTEP * slowdown;
      tick++;
    }

    // render TODO: this should all live inside the view for various screens, let them handle it
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, w, h);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(passthrough_shader);

    // TODO: only get this once on load shader
    GLuint u_perspective = glGetUniformLocation(passthrough_shader, "u_projTrans");
    JGL::setOrthoProjectionMatrix(u_perspective, 0, w, 0, h);
    char fps[100];
    sprintf(fps, "%f, %f", delta_time * 1000.0, 1.0 / delta_time);
    JGL::drawText(fps, g_font, 20, w - 200, h - 20, 1.0, 0.0, 1.0, 1.0);
    JGL::Vec2 guh[] = {{0, 0}, {20, 20}, {60, 20}, {20, 90}};
    JGL::drawLineLoop(guh, 4, 1, 1, 1, 1);

    SDL_GL_SwapWindow(window);
  }
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  deInitSDL(window, &context);
  return 0;
}

int deInitSDL(SDL_Window* window, SDL_GLContext* context) {
  SDL_GL_DeleteContext(*context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

void initJoysticks() {
  int num_joy, i;
  num_joy = SDL_NumJoysticks();
  printf("%d joysticks found\n", num_joy);
  for (i = 0; i < num_joy; i++) {
    SDL_Joystick* joystick = SDL_JoystickOpen(i);
    if (joystick == NULL) {
      printf("Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError());
    } else {
      printf("%s\n", SDL_JoystickName(joystick));
    }
  }
}

bool DebugInput(SDL_Event* event, SDL_Window* window) {
  bool done = false;
  if (event->type == SDL_QUIT ||
      (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_CLOSE &&
       event->window.windowID == SDL_GetWindowID(window))) {
    done = true;
  }
  // TEMPORARY DEBUG KEYS
  if (event->type == SDL_KEYDOWN) {
    switch (event->key.keysym.sym) {
      case SDLK_v:
        g_vsync = !g_vsync;
        SDL_GL_SetSwapInterval(g_vsync);
        break;
    }
  }
  if (event->type == SDL_JOYDEVICEADDED || event->type == SDL_JOYDEVICEREMOVED) {
    initJoysticks();
  }
  return done;
}