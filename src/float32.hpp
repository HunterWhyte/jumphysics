#pragma once
extern "C" {
#include <softfloat/include/softfloat.h>
}

#define F32_M_E float32(2.7182818284590452354f)
#define F32_M_LOG2E float32(1.4426950408889634074f)
#define F32_M_LOG10E float32(0.43429448190325182765f)
#define F32_M_LN2 float32(_M_LN2)
#define F32_M_LN10 float32(2.30258509299404568402f)
#define F32_M_PI float32(3.14159265358979323846f)
#define F32_M_PI_2 float32(1.57079632679489661923f)
#define F32_M_PI_4 float32(0.78539816339744830962f)
#define F32_M_1_PI float32(0.31830988618379067154f)
#define F32_M_2_PI float32(0.63661977236758134308f)
#define F32_M_2_SQRTPI float32(1.12837916709551257390f)
#define F32_M_SQRT2 float32(1.41421356237309504880f)
#define F32_M_SQRT1_2 float32(0.70710678118654752440f)
#define F32_M_2PI float32(6.28318530717958647692f)
#define F32_MAX float32(3.40282347e+37F);

struct float32 {
  float32_t v;

  // Empty constructor --> initialize to positive zero.
  inline float32() { v = ui32_to_f32((uint32_t)0); }

  // Constructor from wrapped type T
  inline float32(const float32_t& v) : v(v) {}

  // Constructor from regular float
  inline float32(float w) {
    // we just want to manually set the bits of v to be equal to the bits of the float
    const uint32_t* value = reinterpret_cast<const uint32_t*>(&w);
    v.v = *value;
  }

  inline float32(uint32_t w) { v = ui32_to_f32(w); }

  inline float32(int32_t w) { v = i32_to_f32(w); }

  // cast back to regular float
  inline explicit operator float() const {
    uint32_t temp = v.v;
    float* result = reinterpret_cast<float*>(&temp);
    return *result;
  }

  // cast to softfloat type
  inline operator float32_t() const { return v; }

  // arithmetic operator overloads

  inline float32 operator-() const { return f32_sub(ui32_to_f32((uint32_t)0), v); }

  inline bool operator==(const float32& b) const { return f32_eq(v, b.v); }

  inline bool operator!=(const float32& b) const { return !(f32_eq(v, b.v)); }

  inline bool operator>(const float32& b) const { return !(f32_le(v, b.v)); }

  inline bool operator<(const float32& b) const { return f32_lt(v, b.v); }

  inline bool operator>=(const float32& b) const { return !(f32_lt(v, b.v)); }

  inline bool operator<=(const float32& b) const { return f32_le(v, b.v); }

  friend inline float32 operator+(const float32& a, const float32& b) { return f32_add(a.v, b.v); }

  friend inline float32 operator-(const float32& a, const float32& b) { return f32_sub(a.v, b.v); }

  friend inline float32 operator*(const float32& a, const float32& b) { return f32_mul(a.v, b.v); }

  friend inline float32 operator/(const float32& a, const float32& b) { return f32_div(a.v, b.v); }

  friend inline float32 operator%(const float32& a, const float32& b) { return f32_rem(a.v, b.v); }

  friend inline float32 sqrt(const float32& a) { return f32_sqrt(a.v); }

  inline float32& operator+=(const float32& b) { return *this = *this + b; }

  inline float32& operator-=(const float32& b) { return *this = *this - b; }

  inline float32& operator*=(const float32& b) { return *this = *this * b; }

  inline float32& operator/=(const float32& b) { return *this = *this / b; }
};

static inline float32 abs(const float32& a) {
  return a > (float32(0)) ? a : -a;
}

float32 f32_sin(float32 a);
float32 f32_cos(float32 a);
float32 f32_tan(float32 a);

float32 f32_atan(float32 a);
float32 f32_atan2(float32 y, float32 x);