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

#include "math.h"
#include "math_util.h"

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

#define CIRCLE_SEGMENTS 11
static void drawCircle(Vec2 center, float size, float r, float g, float b, float a) {
  Vec2 vertices[CIRCLE_SEGMENTS + 1];

  for (int i = 0; i <= CIRCLE_SEGMENTS; i++) {
    float angle = i * 2 * 3.1415 / CIRCLE_SEGMENTS;
    float x = center.x + (size * cosf(angle));
    float y = center.y + (size * sinf(angle));
    vertices[i].x = x;
    vertices[i].y = y;
  }

  JGL::drawTriangleFan(vertices, CIRCLE_SEGMENTS + 1, r, g, b, a);
}

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
  GLuint tex = JGL::loadTexture("../assets/test.png");
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
    // JGL::drawText(fps, g_font, 40, 20, 20, 1.0, 1.0, 1.0, 1.0);

    int body_len = 4;
    float size = 100;
    float l = (w - size) / 2;
    float t = (h - size) / 2;
    Vec2 body[body_len] = {
        {l, t}, {l, t + size}, {l + size, t + size}, {l + size, t}};

    // draw some polygon
    JGL::drawLineLoop(body, 4, 2, 1, 1, 1, 1);

    // draw a dot that follows the mouse
    int mousex, mousey;
    SDL_GetMouseState(&mousex, &mousey);
    Vec2 mouse_pos = {(float)mousex, (float)mousey};

    float collided = true;
    // draw dotted lines extending from the edges of that polygon
    // check overlap for faces of shape A and shape B
    for (int i = 0; i < body_len; i++) {
      Vec2 axis = body[(i + 1) % body_len] - body[i];
      Vec2 offset = body[i];

      // lazy, extend them out some big distance
      Vec2 line[2] = {offset, (offset - (50.0f * axis))};
      JGL::drawLineLoop(line, 2, 1, 1, 1, 1, 0.5);
      Vec2 other_line[2] = {offset, (offset + (50.0f * axis))};
      JGL::drawLineLoop(other_line, 2, 1, 1, 1, 1, 0.5);

      axis = normalize(axis);

      // determine the shapes projection onto the axis
      float max_p;
      float min_p;
      // set the bounds off first vertice
      max_p = dot(axis, body[0] - offset);
      min_p = max_p;
      for (int j = 1; j < body_len; j++) {
        float p = dot(axis, body[j] - offset);
        if (p < min_p) {
          min_p = p;
        } else if (p > max_p) {
          max_p = p;
        }
      }

      Vec2 poly_proj[2] = {offset + (min_p * axis), offset + (max_p * axis)};
      // draw the line segment in a different color between min p and max p
      JGL::drawLineLoop(poly_proj, 2, 1, 1, 0, 0, 0.5);

      // draw dot of the mouse projected onto all the edges of the polygon
      float p = dot(mouse_pos - offset, axis);
      if (p >= min_p && p < max_p) {
        drawCircle(offset + (p * axis), 5, 1, 0, 0, 0.5);
      } else {
        drawCircle(offset + (p * axis), 5, 0, 1, 0, 0.5);
        collided = false;
      }
    }

    // change color if mouse is inside the polygon
    if (collided) {
      drawCircle(mouse_pos, 5, 1, 0, 0, 1);
    } else {
      drawCircle(mouse_pos, 5, 0, 1, 0, 1);
    }

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