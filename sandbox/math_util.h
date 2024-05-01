#pragma once

#include <math.h>

struct Vec2 {

  Vec2() {}
  Vec2(float x, float y) : x(x), y(y) {}

  // overload negate
  Vec2 operator-() const { return Vec2(-x, -y); }

  void operator+=(const Vec2& v) {
    x += v.x;
    y += v.y;
  }

  void operator-=(const Vec2& v) {
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
  Vec2 column1, column2;
};

// Vec2 binary operators
inline Vec2 operator+(const Vec2& a, const Vec2& b) {
  return Vec2(a.x + b.x, a.y + b.y);
}
inline Vec2 operator-(const Vec2& a, const Vec2& b) {
  return Vec2(a.x - b.x, a.y - b.y);
}

inline float dot(const Vec2& a, const Vec2& b) {
  return a.x * b.x + a.y * b.y;
}
inline float cross(const Vec2& a, const Vec2& b) {
  return a.x * b.y - a.y * b.x;
}
inline Vec2 cross(const Vec2& a, float s) {
  return Vec2(s * a.y, -s * a.x);
}
inline Vec2 cross(float s, const Vec2& a) {
  return Vec2(-s * a.y, s * a.x);
}
inline Vec2 operator*(float s, const Vec2& v) {
  return Vec2(s * v.x, s * v.y);
}
inline float distanceSquared(const Vec2& a, const Vec2& b) {
  Vec2 d = b - a;
  return dot(d, d);
}
inline float distance(const Vec2& a, const Vec2& b) {
  return sqrt(distanceSquared(a, b));
}

inline float magnitude(const Vec2& v) {
  return sqrt(v.x * v.x + v.y * v.y);
}
inline Vec2 normalize(const Vec2& a, const Vec2& b) {
  Vec2 normal;
  normal.x = -(b.y - a.y);
  normal.y = b.x - a.x;
  float mag = magnitude(normal);
  normal.x /= mag;
  normal.y /= mag;

  return normal;
}
inline Vec2 normalize(const Vec2& v) {
  return (float(1.0f) / magnitude(v)) * v;
}

inline Vec2 mul(const mat22& A, const Vec2& v) {
  return Vec2(A.column1.x * v.x + A.column2.x * v.y, A.column1.y * v.x + A.column2.y * v.y);
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