/* Copyright (c) 2023  Hunter Whyte */
#ifndef GL_UTIL_H_
#define GL_UTIL_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __EMSCRIPTEN__
#include <SDL_opengles2.h>
#else
#include <glew.h>
// glew has to be included first
#include <SDL_opengl.h>
#include <glu.h>
#endif

#include "stb_image.h"
#include "stb_truetype.h"

#include "math_util.h"

#define NUM_ASCII_CHARS 96
#define FIRST_ASCII_CHAR 32
#define FONT_MAX_LINE_HEIGHT 256.0f
#define FONT_NUM_LEVELS 5
// TODO: for font we want origin of drawing to be corner not the "line" that text is being written on
// this is hacky fix to make the origin approximate the corner
#define ORIGIN_OFFSET 0.65
namespace JGL {

struct NormalizedCharData {
  // normalized st texcoords in glyph map
  float stx, sty, stw, sth;
  // drawing offsets normalized to line height
  float xoff, yoff, xadvance, w, h;
};

struct Font {
  GLuint texture[FONT_NUM_LEVELS];
  NormalizedCharData data[FONT_NUM_LEVELS][NUM_ASCII_CHARS];
};

void init();
GLuint loadShader(GLenum type, const char* shader_src);
GLuint loadShaderProgram(const char* vert_shader_path, const char* frag_shader_path);
GLuint loadTexture(const char* image_path);
void setOrthoProjectionMatrix(GLuint matrix_uniform, GLfloat left, GLfloat right, GLfloat top,
                              GLfloat bottom);
void drawTexturedQuad(GLuint texture, float x, float y, float dest_w, float dest_h, float u,
                      float v, float src_w, float src_h, float r, float g, float b, float a);
void drawLineLoop(const vec2 vertices[], int vertices_len, float r, float g, float b, float a);
void drawLineLoop(const vec2 points[], int points_len, float width, float r, float g, float b, float a);
void drawPoints(const vec2 vertices[], int vertices_len, float r, float g, float b, float a);
void drawTriangleStrip(const vec2 vertices[], int vertices_len, float r, float g, float b, float a);
void drawTriangleFan(const vec2 vertices[], int vertices_len, float r, float g, float b, float a);
void drawTriangles(const vec2 vertices[], int vertices_len, float r, float g, float b, float a);

long loadFont(const char* font_path, Font* font);
void drawText(const char* text, Font font, float line_height, float x, float y, float r, float g,
              float b, float a);
}  // namespace JGL

void checkOpenGLError(const char* stmt, const char* fname, int line);

#if 1
#define GL_CHECK(stmt)                           \
  do {                                           \
    stmt;                                        \
    checkOpenGLError(#stmt, __FILE__, __LINE__); \
  } while (0)
#else
#define GL_CHECK(stmt) stmt
#endif

#endif  // GL_UTIL_H_
