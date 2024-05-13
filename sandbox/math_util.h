#pragma once

#include <math.h>

struct vec2 {

  vec2() {}
  vec2(float x, float y) : x(x), y(y) {}

  // overload negate
  vec2 operator-() const { return vec2(-x, -y); }

  void operator+=(const vec2& v) {
    x += v.x;
    y += v.y;
  }

  void operator-=(const vec2& v) {
    x -= v.x;
    y -= v.y;
  }

  void operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
  }

  float x, y;
};

// 2x2 matrix
struct mat22 {
  mat22() {}
  void set(float angle) {  // rotation transform matrix
    float c = cosf(angle);
    float s = sinf(angle);
    column1 = {c, s};
    column2 = {-s, c};
  }
  vec2 column1, column2;
};

// vec2 binary operators
inline vec2 operator+(const vec2& a, const vec2& b) {
  return vec2(a.x + b.x, a.y + b.y);
}
inline vec2 operator-(const vec2& a, const vec2& b) {
  return vec2(a.x - b.x, a.y - b.y);
}

inline float dot(const vec2& a, const vec2& b) {
  return a.x * b.x + a.y * b.y;
}
inline float cross(const vec2& a, const vec2& b) {
  return a.x * b.y - a.y * b.x;
}
inline vec2 cross(const vec2& a, float s) {
  return vec2(s * a.y, -s * a.x);
}
inline vec2 cross(float s, const vec2& a) {
  return vec2(-s * a.y, s * a.x);
}
inline vec2 operator*(float s, const vec2& v) {
  return vec2(s * v.x, s * v.y);
}
inline float distanceSquared(const vec2& a, const vec2& b) {
  vec2 d = b - a;
  return dot(d, d);
}
inline float distance(const vec2& a, const vec2& b) {
  return sqrt(distanceSquared(a, b));
}

inline float magnitude(const vec2& v) {
  return sqrt(v.x * v.x + v.y * v.y);
}
inline vec2 normalize(const vec2& a, const vec2& b) {
  vec2 normal;
  normal.x = -(b.y - a.y);
  normal.y = b.x - a.x;
  float mag = magnitude(normal);
  normal.x /= mag;
  normal.y /= mag;

  return normal;
}
inline vec2 normalize(const vec2& v) {
  return (float(1.0f) / magnitude(v)) * v;
}

inline vec2 mul(const mat22& A, const vec2& v) {
  return vec2(A.column1.x * v.x + A.column2.x * v.y, A.column1.y * v.x + A.column2.y * v.y);
}

// float operations
inline float max(float a, float b) {
  return a > b ? a : b;
}
inline float min(float a, float b) {
  return a < b ? a : b;
}
inline float clamp(float a, float low, float high) {
  return max(low, min(a, high));
}
inline float sign(float x) {
  return (x < float(0.0f)) ? float(-1.0f) : (x > float(0.0f)) ? float(1.0f) : float(0.0f);
}