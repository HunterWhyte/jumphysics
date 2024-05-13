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
static void drawCircle(vec2 center, float size, float r, float g, float b, float a) {
  vec2 vertices[CIRCLE_SEGMENTS + 1];

  for (int i = 0; i <= CIRCLE_SEGMENTS; i++) {
    float angle = i * 2 * 3.1415 / CIRCLE_SEGMENTS;
    float x = center.x + (size * cosf(angle));
    float y = center.y + (size * sinf(angle));
    vertices[i].x = x;
    vertices[i].y = y;
  }

  JGL::drawTriangleFan(vertices, CIRCLE_SEGMENTS + 1, r, g, b, a);
}

#define MAX_VERTICES 32
struct Body {
  Body() {}
  vec2 center = {0, 0};  // point
  float r = 0;           // angle
  vec2 vertices[MAX_VERTICES];
  int num_vertices = 0;
};

void getAbsoluteVertices(const Body* b, vec2* v) {
  mat22 rot;  // rotation matrix
  rot.set(b->r);
  for (int i = 0; i < b->num_vertices; i++) {
    v[i] = mul(rot, b->vertices[i]);
    v[i] += b->center;
  }
}

float rotator = 0;

#define NUM_SHAPES 6
void drawSim(float w, float h, float delta, int shape) {
  Body b = {};
  float s = min(w, h)/5;
  rotator += delta * 0.1;
  b.center = {w / 2, h / 2};
  switch (shape) {
    case 0:
      b.vertices[0] = {-s, -s};
      b.vertices[1] = {-s, s};
      b.vertices[2] = {s, s};
      b.vertices[3] = {s, -s};
      b.num_vertices = 4;
      break;
    case 1:
      b.vertices[0] = {-s, -s};
      b.vertices[1] = {-s, s};
      b.vertices[2] = {s, s / 2};
      b.num_vertices = 3;
      break;
    case 2:
      b.vertices[0] = {-s * 2, 0};
      b.vertices[1] = {-s / 2, -s};
      b.vertices[2] = {s / 2, -s};
      b.vertices[3] = {s * 2, 0};
      b.vertices[4] = {s / 2, s};
      b.vertices[5] = {-s / 2, s};
      b.num_vertices = 6;
      break;
    case 3:
      b.vertices[0] = {-s * 0.5f, -s * 1.2f};
      b.vertices[1] = {-s * 1.1f, 0.0f};
      b.vertices[2] = {-s * 0.5f, s * 1.2f};
      b.vertices[3] = {s * 0.3f, s * 0.8f};
      b.vertices[4] = {s * 1.3f, 0.0f};
      b.vertices[5] = {s * 0.3f, -s * 0.8f};
      b.num_vertices = 6;
      break;
    case 4:
      b.vertices[0] = {-s * 0.8f, -s};
      b.vertices[1] = {-s, s * 0.6f};
      b.vertices[2] = {0.0f, s*2.0f};
      b.vertices[3] = {s, s * 3.0f};
      b.vertices[4] = {s * 1.8f, -s};
      b.num_vertices = 5;
      break;
    case 5:
      b.vertices[0] = {-s, -s * 0.3f};
      b.vertices[1] = {-s * 0.7f, s * 0.9f};
      b.vertices[2] = {0.0f, s};
      b.vertices[3] = {s * 0.7f, s * 0.9f};
      b.vertices[4] = {s, -s * 0.3f};
      b.vertices[5] = {0.0f, -s * 3.1f};
      b.num_vertices = 6;
      break;
  }

  b.r = rotator;

  vec2 verts[MAX_VERTICES];
  getAbsoluteVertices(&b, verts);

  // draw a dot that follows the mouse
  int mousex, mousey;
  SDL_GetMouseState(&mousex, &mousey);
  vec2 mouse_pos = {(float)mousex, (float)mousey};

  float collided = true;
  // draw dotted lines extending from the edges of that polygon
  // check overlap for faces of shape A and shape B
  for (int i = 0; i < b.num_vertices; i++) {
    vec2 edge = verts[(i + 1) % b.num_vertices] - verts[i];
    vec2 axis = {edge.y, -edge.x};
    vec2 offset = verts[i] + (0.5 * edge);

    // lazy, extend them out some big distance
    vec2 line[2] = {offset, (offset - (50.0f * axis))};
    JGL::drawLineLoop(line, 2, 1, 1, 1, 1, 0.15);
    vec2 other_line[2] = {offset, (offset + (50.0f * axis))};
    JGL::drawLineLoop(other_line, 2, 1, 1, 1, 1, 0.15);

    axis = normalize(axis);

    // determine the shapes projection onto the axis
    float max_p;
    float min_p;
    // set the bounds off first vertice
    max_p = dot(axis, verts[0] - offset);
    min_p = max_p;
    for (int j = 1; j < b.num_vertices; j++) {
      float p = dot(axis, verts[j] - offset);
      if (p < min_p) {
        min_p = p;
      } else if (p > max_p) {
        max_p = p;
      }
    }

    // draw the line segment in a different color between min p and max p
    vec2 poly_proj[2] = {offset + (min_p * axis), offset + (max_p * axis)};
    JGL::drawLineLoop(poly_proj, 2, 2, 1, 0, 0, 0.2);

    // draw dot of the mouse projected onto all the edges of the polygon
    float p = dot(mouse_pos - offset, axis);
    if (p >= min_p && p < max_p) {
      drawCircle(offset + (p * axis), 4, 1, 0, 0, 0.5);
    } else {
      drawCircle(offset + (p * axis), 4, 0, 1, 0, 0.5);
      collided = false;
    }
  }

  // change color if mouse is inside the polygon
  if (collided) {
    JGL::drawLineLoop(verts, b.num_vertices, 2, 1, 0, 0, 1);
    drawCircle(mouse_pos, 6, 1, 0, 0, 1);
  } else {
    JGL::drawLineLoop(verts, b.num_vertices, 2, 1, 1, 1, 1);
    drawCircle(mouse_pos, 6, 0, 1, 0, 1);
  }
}

#define NUM_SHAPES 6
void polyToPoly(float w, float h, float delta, int shape) {
  Body a = {};
  Body b = {};
  float s = min(w, h) / 8;
  rotator += delta * 0.1;
  b.center = {w / 2, h / 2};
  switch (shape) {
    case 0:
      b.vertices[0] = {-s, -s};
      b.vertices[1] = {-s, s};
      b.vertices[2] = {s, s};
      b.vertices[3] = {s, -s};
      b.num_vertices = 4;
      break;
    case 1:
      b.vertices[0] = {-s, -s};
      b.vertices[1] = {-s, s};
      b.vertices[2] = {s, s / 2};
      b.num_vertices = 3;
      break;
    case 2:
      b.vertices[0] = {-s * 2, 0};
      b.vertices[1] = {-s / 2, -s};
      b.vertices[2] = {s / 2, -s};
      b.vertices[3] = {s * 2, 0};
      b.vertices[4] = {s / 2, s};
      b.vertices[5] = {-s / 2, s};
      b.num_vertices = 6;
      break;
    case 3:
      b.vertices[0] = {-s * 0.5f, -s * 1.2f};
      b.vertices[1] = {-s * 1.1f, 0.0f};
      b.vertices[2] = {-s * 0.5f, s * 1.2f};
      b.vertices[3] = {s * 0.3f, s * 0.8f};
      b.vertices[4] = {s * 1.3f, 0.0f};
      b.vertices[5] = {s * 0.3f, -s * 0.8f};
      b.num_vertices = 6;
      break;
    case 4:
      b.vertices[0] = {-s * 0.8f, -s};
      b.vertices[1] = {-s, s * 0.6f};
      b.vertices[2] = {0.0f, s * 2.0f};
      b.vertices[3] = {s, s * 3.0f};
      b.vertices[4] = {s * 1.8f, -s};
      b.num_vertices = 5;
      break;
    case 5:
      b.vertices[0] = {-s, -s * 0.3f};
      b.vertices[1] = {-s * 0.7f, s * 0.9f};
      b.vertices[2] = {0.0f, s};
      b.vertices[3] = {s * 0.7f, s * 0.9f};
      b.vertices[4] = {s, -s * 0.3f};
      b.vertices[5] = {0.0f, -s * 3.1f};
      b.num_vertices = 6;
      break;
  }

  a.vertices[0] = {-s, -s};
  a.vertices[1] = {-s, s};
  a.vertices[2] = {s, s / 2};
  a.num_vertices = 3;

  b.r = M_2_PI;

  // draw a dot that follows the mouse
  int mousex, mousey;
  SDL_GetMouseState(&mousex, &mousey);
  vec2 mouse_pos = {(float)mousex, (float)mousey};
  a.center = mouse_pos;

  vec2 b_verts[MAX_VERTICES];
  getAbsoluteVertices(&b, b_verts);
  vec2 a_verts[MAX_VERTICES];
  getAbsoluteVertices(&a, a_verts);

  float collided = true;

  for (int i = 0; i < a.num_vertices + b.num_vertices; i++) {
    vec2 edge;
    vec2 normal;
    vec2 offset;
    if (i < a.num_vertices) {
      int ind = i;
      edge = a_verts[(ind + 1) % a.num_vertices] - a_verts[ind];
      normal = {edge.y, -edge.x};
      offset = a_verts[ind] + (0.5 * edge);
    } else {
      int ind = i - a.num_vertices;
      edge = b_verts[(ind + 1) % b.num_vertices] - b_verts[ind];
      normal = {edge.y, -edge.x};
      offset = b_verts[ind] + (0.5 * edge);
    }

    vec2 axis = normalize(normal);

    // determine shape A projection onto the axis
    float a_max_p;
    float a_min_p;
    // set the bounds off first vertice
    a_max_p = dot(axis, a_verts[0] - offset);
    a_min_p = a_max_p;
    for (int j = 1; j < a.num_vertices; j++) {
      float p = dot(axis, a_verts[j] - offset);
      if (p < a_min_p) {
        a_min_p = p;
      } else if (p > a_max_p) {
        a_max_p = p;
      }
    }
    // shape B projection onto the axis
    float b_max_p;
    float b_min_p;
    // set the bounds off first vertice
    b_max_p = dot(axis, b_verts[0] - offset);
    b_min_p = b_max_p;
    for (int j = 1; j < b.num_vertices; j++) {
      float p = dot(axis, b_verts[j] - offset);
      if (p < b_min_p) {
        b_min_p = p;
      } else if (p > b_max_p) {
        b_max_p = p;
      }
    }

    float overlap = min(a_max_p, b_max_p) - max(a_min_p, b_min_p);

    // if overlapping
    if (overlap > 0) {
      // draw the axis lazy, extend them out some big distance
      vec2 line[2] = {offset, (offset - (50.0f * normal))};
      JGL::drawLineLoop(line, 2, 1, 1, 0, 0, 0.4);
      vec2 other_line[2] = {offset, (offset + (50.0f * normal))};
      JGL::drawLineLoop(other_line, 2, 1, 1, 0, 0, 0.4);
    } else {
      collided = false;
      vec2 line[2] = {offset, (offset - (50.0f * normal))};
      JGL::drawLineLoop(line, 2, 1, 1, 1, 1, 0.15);
      vec2 other_line[2] = {offset, (offset + (50.0f * normal))};
      JGL::drawLineLoop(other_line, 2, 1, 1, 1, 1, 0.15);
    }

    // draw the line segment in a different color between min p and max p
    vec2 a_poly_proj[2] = {offset + (a_min_p * axis), offset + (a_max_p * axis)};
    JGL::drawLineLoop(a_poly_proj, 2, 2, 0.2, 0.2, 1, 0.5);

    // draw the line segment in a different color between min p and max p
    vec2 b_poly_proj[2] = {offset + (b_min_p * axis), offset + (b_max_p * axis)};
    JGL::drawLineLoop(b_poly_proj, 2, 2, 0, 1, 0, 0.2);
  }

  if (collided) {
    JGL::drawLineLoop(b_verts, b.num_vertices, 2, 1, 0, 0, 0.75);
    JGL::drawLineLoop(a_verts, a.num_vertices, 2, 1, 0, 0, 0.75);
  } else {
    JGL::drawLineLoop(b_verts, b.num_vertices, 2, 1, 1, 1, 0.75);
    JGL::drawLineLoop(a_verts, a.num_vertices, 2, 1, 1, 1, 0.75);
  }
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
  int shape = 3;
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
        if (event.type == SDL_MOUSEBUTTONDOWN) {
          shape++;
          if (shape == NUM_SHAPES) {
            shape = 0;
          }
        }
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

    drawSim((float)w, (float)h, delta_time, shape);
    // polyToPoly((float)w, (float)h, delta_time, shape);

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