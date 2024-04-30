#ifndef COLLISION_H
#define COLLISION_H
#include <assert.h>
#include "math_util.h"

#define MAX_VERTICES 8

// simplex vertex of Minkowski difference
struct simplex_vertex {
  vec2 point_a;  // support point from polygon A
  int index_a;   // index of support point in polygon A

  vec2 point_b;  // support point from polygon B
  int index_b;   // index of support point in polygon A

  vec2 point;  // final Minkowski difference support point
  float32 b_coord;  // unnormalized barycentric coordinate of target relative to this vertex
};

// feature, edge or vertex
struct feature {
  int index_1, index_2;
  bool edge;
};

struct body {
  body() {}

  vec2 center = {float32(0), float32(0)};  // point
  vec2 vel = {float32(0), float32(0)};
  float32 w = float32(0);  // angular velocity
  float32 r = float32(0);  // angle
  vec2 vertices[MAX_VERTICES];
  int num_vertices = 0;
  float32 inv_mass = float32(0);
  float32 inv_I = float32(0);
  float32 friction = float32(0);
};

bool continuous_collision(const body* body_a, const body* body_b, float32* impact_time, feature* fa,
                         feature* fb, vec2* impact, float32 start_time);
// GJK
float32 polygon_distance(const vec2* polygon_a, int len_a, const vec2* polygon_b, int len_b,
                      vec2* closest_a, vec2* closest_b, feature* feature_a, feature* feature_b);
bool line_segment_intersect(vec2 a0, vec2 a1, vec2 b0, vec2 b1, vec2* intersection, float32* ta,
                          float32* tb);
bool separating_axis_intersect(const vec2 a[], int a_len, const vec2 b[], int b_len,
                             vec2* minimum_vector, float32* minimum_overlap);
void handle_collision(body* body_a, body* body_b, feature fa, feature fb, vec2 impact, float32 t,
                     float32 restitution);


// get vertices translated to center with angle r
// t is return parameter vec2* with length of at least num_vertices
void get_absolute_vertices(const body* b, vec2* v);
void get_absolute_vertices(const body* b, vec2* v, float32 t);
vec2 get_absolute_vertex(const body* b, int index, float32 t);
vec2 get_absolute_vertex(const body* b, int index);
vec2 get_center(const body* b, float32 t);

#endif  // COLLISION_H