#pragma once
#include "float32.hpp"

struct vec2 {

  vec2() {}
  vec2(float32 x, float32 y) : x(x), y(y) {}

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

  void operator*=(float32 scalar) {
    x *= scalar;
    y *= scalar;
  }

  float32 x, y;
};

// 2x2 matrix
struct mat22 {
  mat22() {}
  void set(float32 angle) {  // rotation transform matrix
    float32 c = f32_cos(angle);
    float32 s = f32_sin(angle);
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

inline float32 dot(const vec2& a, const vec2& b) {
  return a.x * b.x + a.y * b.y;
}
inline float32 cross(const vec2& a, const vec2& b) {
  return a.x * b.y - a.y * b.x;
}
inline vec2 cross(const vec2& a, float32 s) {
  return vec2(s * a.y, -s * a.x);
}
inline vec2 cross(float32 s, const vec2& a) {
  return vec2(-s * a.y, s * a.x);
}
inline vec2 operator*(float32 s, const vec2& v) {
  return vec2(s * v.x, s * v.y);
}
inline float32 distanceSquared(const vec2& a, const vec2& b) {
  vec2 d = b - a;
  return dot(d, d);
}
inline float32 distance(const vec2& a, const vec2& b) {
  return sqrt(distanceSquared(a, b));
}

inline float32 magnitude(const vec2& v) {
  return sqrt(v.x * v.x + v.y * v.y);
}
inline vec2 normalize(const vec2& a, const vec2& b) {
  vec2 normal;
  normal.x = -(b.y - a.y);
  normal.y = b.x - a.x;
  float32 mag = magnitude(normal);
  normal.x /= mag;
  normal.y /= mag;

  return normal;
}
inline vec2 normalize(const vec2& v) {
  return (float32(1.0f) / magnitude(v)) * v;
}

inline vec2 mul(const mat22& A, const vec2& v) {
  return vec2(A.column1.x * v.x + A.column2.x * v.y, A.column1.y * v.x + A.column2.y * v.y);
}

// float32 operations
inline float32 max(float32 a, float32 b) {
  return a > b ? a : b;
}
inline float32 min(float32 a, float32 b) {
  return a < b ? a : b;
}
inline float32 clamp(float32 a, float32 low, float32 high) {
  return max(low, min(a, high));
}
inline float32 sign(float32 x) {
  return (x < float32(0.0f)) ? float32(-1.0f) : (x > float32(0.0f)) ? float32(1.0f) : float32(0.0f);
}