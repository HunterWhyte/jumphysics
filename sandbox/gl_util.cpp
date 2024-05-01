#include <gl_util.h>

using namespace JGL;

GLuint white;

void checkOpenGLError(const char* stmt, const char* fname, int line)
{
    GLenum err = glGetError();
    while(err != GL_NO_ERROR)
    {
        printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        err = glGetError();
        // abort();
    }
}

void JGL::init() {
  unsigned char image_data[] = {0xFF, 0xFF, 0xFF, 0xFF};
  GL_CHECK(glGenTextures(1, &white));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, white));

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
  GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data));
}

long readEntireFile(const char* filepath, unsigned char** buffer) {
  long length;
  FILE* file;

  file = fopen(filepath, "rb");
  if (file) {
    // find length of file so that we can allocate buffer to fit
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    *buffer = (unsigned char*)malloc(length);
    fread(*buffer, 1, length, file);
    fclose(file);
    return length;
  } else {
    printf("failed to open file %s\n", filepath);
    return 0;
  }
}

/* read and load font into texture, returns 0 on success */
long JGL::loadFont(const char* font_path, Font* font) {
  long length;
  unsigned char* font_buffer;
  float font_height;
  int bw;
  int bh;
  unsigned char* bitmap;
  unsigned char* colored_bitmap;
  stbtt_bakedchar charinfo[NUM_ASCII_CHARS];

  length = readEntireFile(font_path, &font_buffer);
  if (!length) {
    printf("failed to read font file\n");
    return 1;
  }

  /* prepare font */
  stbtt_fontinfo info;
  if (!stbtt_InitFont(&info, font_buffer, 0)) {
    printf("failed to initialize font\n");
    return 1;
  }

  // generate the rest of the textures
  for (int i = 0; i < FONT_NUM_LEVELS; i++) {
    // get texture handle and setup parameters
    GL_CHECK(glGenTextures(1, &font->texture[i]));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, font->texture[i]));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    // determine bitmap size
    font_height = FONT_MAX_LINE_HEIGHT / (float)(1 << i);
    bw = font_height * 8;
    bh = font_height * 8;
    bitmap = (unsigned char*)malloc(bw * bh);
    colored_bitmap = (unsigned char*)malloc(bw * bh * 4);
    stbtt_BakeFontBitmap(font_buffer, 0, font_height, bitmap, bw, bh, FIRST_ASCII_CHAR,
                         NUM_ASCII_CHARS, charinfo);

    // stb outputs bitmap as single channel which we can give to glTexImage2D as alpha
    // however, glTexImage2D then fills R,G,B with 0 making text black and ruining our recoloring
    // method in the shader, therefore we create a new buffer and fill RGB with all ones so that
    // the texture has white characters instead of black
    for (int k = 0; k < (bw * bh); k++) {
      colored_bitmap[k * 4 + 0] = 0xFF;
      colored_bitmap[k * 4 + 1] = 0xFF;
      colored_bitmap[k * 4 + 2] = 0xFF;
      colored_bitmap[k * 4 + 3] = bitmap[k];
    }
    // set texture image
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bw, bh, 0, GL_RGBA, GL_UNSIGNED_BYTE, colored_bitmap));
    free(bitmap);
    free(colored_bitmap);

    // store normalized glyph data for all ASCII chars for each level
    for (int j = 0; j < NUM_ASCII_CHARS; j++) {
      font->data[i][j].stx = ((float)charinfo[j].x0) / ((float)bw);
      font->data[i][j].sty = ((float)charinfo[j].y0) / ((float)bh);
      font->data[i][j].stw = ((float)charinfo[j].x1 - charinfo[j].x0) / ((float)bw);
      font->data[i][j].sth = ((float)charinfo[j].y1 - charinfo[j].y0) / ((float)bh);

      font->data[i][j].xoff = charinfo[j].xoff / font_height;
      font->data[i][j].yoff = charinfo[j].yoff / font_height;
      font->data[i][j].xadvance = charinfo[j].xadvance / font_height;
      font->data[i][j].w = ((float)charinfo[j].x1 - charinfo[j].x0) / font_height;
      font->data[i][j].h = ((float)charinfo[j].y1 - charinfo[j].y0) / font_height;
    }
  }

  free(font_buffer);

  return 0;
}

void JGL::drawText(const char* text, Font font, float line_height, float origin_x, float origin_y,
                   float r, float g, float b, float a) {
  GLuint texture;
  NormalizedCharData* data;
  // determine scaling level to use, nearest power of two to line height
  int level = FONT_NUM_LEVELS - 1;
  for (int i = 1; i < FONT_NUM_LEVELS; i++) {
    if (line_height > FONT_MAX_LINE_HEIGHT / (1 << i)) {
      level = i - 1;
      break;
    }
  }
  texture = font.texture[level];
  data = font.data[level];
  float advanced_x = origin_x;  // x keeping track of letter advancing
  int i = 0;
  while (text[i] != '\0') {
    NormalizedCharData d = data[text[i] - 32];
    float x = advanced_x + (d.xoff * line_height);
    float y = origin_y + ((ORIGIN_OFFSET + d.yoff) * line_height);
    drawTexturedQuad(texture, x, y, d.w * line_height, d.h * line_height, d.stx, d.sty, d.stw,
                     d.sth, r, g, b, a);
    advanced_x += d.xadvance * line_height;
    i++;
  }
}

GLuint JGL::loadShader(GLenum type, const char* shader_path) {
  GLuint shader;
  GLint compiled;
  GLint length;
  unsigned char* buffer;
  char* shader_src = 0;  // buffer for shader source

  length = readEntireFile(shader_path, &buffer);
  if (!length) {
    printf("failed to read shader file\n");
    return 0;
  }
  shader_src = (char*)buffer;

  // Create the shader object
  GL_CHECK(shader = glCreateShader(type));
  if (!shader) {
    printf("failed to create shader object. Max # of shader objects likely reached\n");
    free(shader_src);
    return 0;
  }

  GL_CHECK(glShaderSource(shader, 1, &shader_src, &length));
  GL_CHECK(glCompileShader(shader));

  GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));
  if (!compiled) {
    GLint info_len = 0;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len > 1) {
      char* infoLog = (char*)malloc(sizeof(char) * info_len);
      glGetShaderInfoLog(shader, info_len, NULL, infoLog);
      printf("Error compiling shader %s:\n%s\n", shader_path, infoLog);
      free(infoLog);
    }
    glDeleteShader(shader);
    free(shader_src);
    return 0;
  }

  free(shader_src);
  return shader;
}

GLuint JGL::loadTexture(const char* image_path) {
  int image_width = 0;
  int image_height = 0;
  int channels;
  GLuint texture;
  unsigned char* image_data;
  image_data = stbi_load(image_path, &image_width, &image_height, &channels, STBI_rgb_alpha);

  if (image_data == nullptr) {
    printf("File not found %s", image_path);
    return 0;
  }

  GL_CHECK(glGenTextures(1, &texture));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));

  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
  GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               image_data));
  free(image_data);

  return texture;
}

GLuint JGL::loadShaderProgram(const char* vert_shader_path, const char* frag_shader_path) {
  GLuint vertex_shader;
  GLuint fragment_shader;
  GLuint program_object;
  GLint linked;

  // Load the vertex/fragment shaders
  vertex_shader = JGL::loadShader(GL_VERTEX_SHADER, vert_shader_path);
  if (vertex_shader == 0)
    return 0;

  fragment_shader = JGL::loadShader(GL_FRAGMENT_SHADER, frag_shader_path);
  if (fragment_shader == 0) {
    glDeleteShader(vertex_shader);
    return 0;
  }

  // Create the program object
  GL_CHECK(program_object = glCreateProgram());

  if (program_object == 0)
    return 0;

  GL_CHECK(glAttachShader(program_object, vertex_shader));
  GL_CHECK(glAttachShader(program_object, fragment_shader));

  GL_CHECK(glLinkProgram(program_object));
  GL_CHECK(glGetProgramiv(program_object, GL_LINK_STATUS, &linked));
  if (!linked) {
    GLint info_len = 0;

    GL_CHECK(glGetProgramiv(program_object, GL_INFO_LOG_LENGTH, &info_len));
    if (info_len > 1) {
      char* infoLog = (char*)malloc(sizeof(char) * info_len);
      glGetProgramInfoLog(program_object, info_len, NULL, infoLog);
      printf("Error linking program:\n%s\n", infoLog);
      free(infoLog);
    }

    glDeleteProgram(program_object);
    return 0;
  }

  // Free up no longer needed shader resources
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return program_object;
}

void JGL::setOrthoProjectionMatrix(GLuint matrix_uniform, GLfloat left, GLfloat right, GLfloat top,
                                   GLfloat bottom) {
  float ortho_projection[4][4] = {
      {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
      {0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
      {0.0f, 0.0f, -1.0f, 0.0f},
      {(right + left) / (left - right), (top + bottom) / (bottom - top), 0.0f, 1.0f},
  };
  glUniformMatrix4fv(matrix_uniform, 1, GL_FALSE, &ortho_projection[0][0]);
}

// TODO: make macros to define and explain all these values
void JGL::drawTexturedQuad(GLuint texture, float x, float y, float dest_w, float dest_h, float u,
                           float v, float src_w, float src_h, float r, float g, float b, float a) {
  GLfloat vertices[36];
  GLushort indices[] = {0, 1, 2, 0, 2, 3};
  // four vertices
  // top left
  vertices[0] = x;     // x0
  vertices[1] = y;     // y0
  vertices[2] = 0.0f;  // z0
  vertices[3] = u;     // u0
  vertices[4] = v;     // v0
  vertices[5] = r;
  vertices[6] = g;
  vertices[7] = b;
  vertices[8] = a;

  // bottom left
  vertices[9] = x;            //x1
  vertices[10] = y + dest_h;  //y1
  vertices[11] = 0.0f;        // z1
  vertices[12] = u;           // u1
  vertices[13] = v + src_h;   // v1
  vertices[14] = r;
  vertices[15] = g;
  vertices[16] = b;
  vertices[17] = a;

  // bottom right
  vertices[18] = x + dest_w;  // x2
  vertices[19] = y + dest_h;  // y2
  vertices[20] = 0.0f;        // z2
  vertices[21] = u + src_w;   // u2
  vertices[22] = v + src_h;   // v2
  vertices[23] = r;
  vertices[24] = g;
  vertices[25] = b;
  vertices[26] = a;

  // top right
  vertices[27] = x + dest_w;  // x3
  vertices[28] = y;           // y3
  vertices[29] = 0.0f;        // z3
  vertices[30] = u + src_w;   // u3
  vertices[31] = v;           // v3
  vertices[32] = r;
  vertices[33] = g;
  vertices[34] = b;
  vertices[35] = a;

  // Load the vertex data
  // earlier we bound vposition to attribute index 0
  // Load the vertex position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), vertices);
  glEnableVertexAttribArray(0);
  // Load the texture coordinate
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), &vertices[3]);
  glEnableVertexAttribArray(1);
  // Load the color values
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), &vertices[5]);
  glEnableVertexAttribArray(2);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  // Set the base map sampler to texture unit to 0
  glUniform1i(0, 0);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void drawPrimitive(GLuint primitive, const Vec2 points[], int points_len, float r, float g, float b,
                   float a) {
  GLfloat vertices[points_len * 9];
  GLushort indices[points_len];

  for (int i = 0; i < points_len; i++) {
    vertices[i * 9 + 0] = (float)points[i].x;  // x
    vertices[i * 9 + 1] = (float)points[i].y;  // y
    vertices[i * 9 + 2] = 0.0f;         // z
    vertices[i * 9 + 3] = 0;            // s
    vertices[i * 9 + 4] = 0;            // t
    vertices[i * 9 + 5] = r;            // r
    vertices[i * 9 + 6] = g;            // g
    vertices[i * 9 + 7] = b;            // b
    vertices[i * 9 + 8] = a;            // a

    indices[i] = i;
  }

  // Load the vertex data
  // earlier we bound vposition to attribute index 0
  // Load the vertex position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), vertices);
  glEnableVertexAttribArray(0);
  // Load the texture coordinate
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), &vertices[3]);
  glEnableVertexAttribArray(1);
  // Load the color values
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), &vertices[5]);
  glEnableVertexAttribArray(2);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, white);
  // Set the base map sampler to texture unit to 0
  glUniform1i(0, 0);
  glDrawElements(primitive, points_len, GL_UNSIGNED_SHORT, indices);
}

void JGL::drawLineLoop(const Vec2 points[], int points_len, float r, float g, float b, float a) {
  drawPrimitive(GL_LINE_LOOP, points, points_len, r, g, b, a);
}

void JGL::drawPoints(const Vec2 points[], int points_len, float r, float g, float b, float a) {
  drawPrimitive(GL_POINTS, points, points_len, r, g, b, a);
}

void JGL::drawTriangleStrip(const Vec2 points[], int points_len, float r, float g, float b, float a) {
  drawPrimitive(GL_TRIANGLE_STRIP, points, points_len, r, g, b, a);
}

void JGL::drawTriangleFan(const Vec2 points[], int points_len, float r, float g, float b, float a) {
  drawPrimitive(GL_TRIANGLE_FAN, points, points_len, r, g, b, a);
}

void JGL::drawTriangles(const Vec2 points[], int points_len, float r, float g, float b, float a) {
  drawPrimitive(GL_TRIANGLES, points, points_len, r, g, b, a);
}