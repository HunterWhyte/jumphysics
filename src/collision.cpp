#include "collision.h"
#include "math_util.h"
#include "stdio.h"

int solveSimplex2(simplex_vertex* simplex, float32* divisor, vec2 target);
int solveSimplex3(simplex_vertex* simplex, float32* divisor, vec2 target);
vec2 getSearchDirection(simplex_vertex* simplex, int simplex_size);
int getSupportPoint(const vec2* p, int len, vec2 d);
float32 getClosestPoints(simplex_vertex* simplex, int simplex_size, float32 divisor, vec2* a,
                         vec2* b);

const float32 tol = float32(0.01f);

bool discreteCollision(const body* body_a, const body* body_b, feature* fa, feature* fb,
                       vec2* impact, float32 t) {
  printf("warning discrete collision\n");
  // use separating axis theorem to move everything back to separated
  // then call GJK to get features and return with time = 0
  vec2 mv;
  float32 md;
  int a_len = body_a->num_vertices;
  int b_len = body_b->num_vertices;
  vec2 polygon_a[a_len];
  vec2 polygon_b[b_len];
  get_absolute_vertices(body_a, polygon_a, t);
  get_absolute_vertices(body_b, polygon_b, t);
  assert(separating_axis_intersect(polygon_a, a_len, polygon_b, b_len, &mv, &md));
  // move the object with a lower mass (infinite mass objects never get moved)
  if (body_a->inv_mass < body_b->inv_mass) {
    for (int i = 0; i < b_len; i++) {
      polygon_b[i] -= (md * float32(1.1f)) * mv;
    }
  } else {
    for (int i = 0; i < a_len; i++) {
      polygon_a[i] += (md * float32(1.1f)) * mv;
    }
  }

  float32 distance = polygon_distance(polygon_a, a_len, polygon_b, b_len, impact, NULL, fa, fb);
  if (distance == 0) {
    printf("warning discrete collision after separating axis distance still 0 %f\n", (float)distance);
  }
  // printf("discrete distance: %f\n", distance);
  return true;
}

// Bilateral advancement algorithm as explained in https://box2d.org/files/ErinCatto_ContinuousCollision_GDC2013.pdf
bool continuous_collision(const body* body_a, const body* body_b, float32* impact_time, feature* fa,
                         feature* fb, vec2* impact, float32 start_time) {
  int a_len = body_a->num_vertices;
  int b_len = body_b->num_vertices;
  vec2 polygon_a[a_len];
  vec2 polygon_b[b_len];
  feature feature_a, feature_b;
  vec2 closest_a, closest_b;
  float32 distance;

  float32 t1 = start_time;
  float32 t2 = 0;

  get_absolute_vertices(body_a, polygon_a, t1);
  get_absolute_vertices(body_b, polygon_b, t1);

  distance = polygon_distance(polygon_a, a_len, polygon_b, b_len, &closest_a, &closest_b, &feature_a,
                             &feature_b);

  // early exit for already overlapping
  if (distance == 0) {
    // at 0 already collided something failed, handle with discrete collision
    vec2 imp;
    discreteCollision(body_a, body_b, &feature_a, &feature_b, &imp, t1);
    *impact_time = t1;
    *fa = feature_a;
    *fb = feature_b;
    *impact = imp;
    return true;
  }

  int iter = 0;
  while (iter < 20) {
    if ((!feature_a.edge) && (!feature_b.edge)) {  // point to point
      // separation function depends on separating axis u which is calculated
      // from feature a to b at time 0 and is fixed
      vec2 a0, b0;
      a0 = get_absolute_vertex(body_a, feature_a.index_1, t1);
      b0 = get_absolute_vertex(body_b, feature_b.index_1, t1);
      vec2 u = b0 - a0;  // seperation axis,
      if (magnitude(u) < tol) {
        *impact_time = t1;
        *impact = b0;
        *fa = feature_a;
        *fb = feature_b;
        return true;  // time of impact is t1
      }
      u = normalize(u);  // length of u is guaranteed != 0 so this is okay

      t2 = 1;

      while (1) {
        // get polygon for selected time
        get_absolute_vertices(body_a, polygon_a, t2);
        get_absolute_vertices(body_b, polygon_b, t2);
        // find deepest points
        int index_a = getSupportPoint(polygon_a, body_a->num_vertices, u);
        int index_b = getSupportPoint(polygon_b, body_b->num_vertices, -u);

        // calculate s
        float32 s = dot(polygon_b[index_b] - polygon_a[index_a], u);
        // printf("point-point separation: %f\n", s);

        if (s > tol) {
          // deepest points are not past the plane, polygons do not collide
          // printf("deepest points are not past the plane, polygons do not collide\n");
          return false;
        } else if (s < -tol) {
          float32 a, b, c;
          a = t1;
          b = t2;
          // root finding by bisection always approaching from positive side
          int b_iter = 0;
          while (1) {
            c = (a + b) / 2;
            get_absolute_vertices(body_a, polygon_a, c);
            get_absolute_vertices(body_b, polygon_b, c);
            s = dot(polygon_b[index_b] - polygon_a[index_a], u);
            // printf("[%d]: s %f, a %f, b %f, c %f\n", b_iter, s, a, b, c);
            if (abs(s) < tol) {  // root found
              break;
            } else if (s > 0) {
              a = c;
            } else {
              b = c;
            }
            b_iter++;
            if (b_iter > 20) {
              printf("warning bisection root finding iterations exceeded");
              return false;
            }
          }
          t2 = c;
        } else {
          // deepest points are within tolerance of touching on separation axis
          t1 = t2;
          break;
        }
      }
    } else {  // point to edge
      const body* body_edge;
      const body* body_point;
      feature feature_edge;
      feature feature_point;
      // redefine bodies and points so that either a or b can be used as either
      if (feature_a.edge && !feature_b.edge) {
        body_edge = body_a;
        feature_edge = feature_a;

        body_point = body_b;
        feature_point = feature_b;
      } else if (feature_b.edge && !feature_a.edge) {
        body_edge = body_b;
        feature_edge = feature_b;

        body_point = body_a;
        feature_point = feature_a;
      } else {
        assert(false);  // should never be given an edge-edge case from polygon_distance
      }
      int point_len = body_point->num_vertices;
      vec2 polygon_point[point_len];

      vec2 edge0, edge1, point;
      edge0 = get_absolute_vertex(body_edge, feature_edge.index_1, t1);
      edge1 = get_absolute_vertex(body_edge, feature_edge.index_2, t1);
      point = get_absolute_vertex(body_point, feature_point.index_1, t1);

      vec2 edge = edge1 - edge0;
      // make normal positive facing out of polygon
      vec2 n = cross(edge, edge0 - get_center(body_edge, t1)) > 0 ? cross(1, edge) : cross(edge, 1);
      n = normalize(n);
      // dot(a0,n) = dot(a1,n) is the offset of the plane in the normal axis from origin
      float32 s = dot(point, n) - dot(edge0, n);

      // printf("edge-point separation: %f, dot(b0, n) = %f, dot(a0, n) = %f \n", s, dot(point, n),
      //        dot(edge0, n));

      if (abs(s) < tol) {
        *impact_time = t1;
        *impact = closest_a;
        *fa = feature_a;
        *fb = feature_b;
        return true;
      }

      t2 = float32(1);
      while (1) {
        // get plane determined earlier at new time t2
        edge0 = get_absolute_vertex(body_edge, feature_edge.index_1, t2);
        edge1 = get_absolute_vertex(body_edge, feature_edge.index_2, t2);
        edge = edge1 - edge0;
        vec2 n =
            cross(edge, edge0 - get_center(body_edge, t2)) > 0 ? cross(1, edge) : cross(edge, 1);
        n = normalize(n);
        // have to get all points of the polygon that doesn't make up the plane
        // in order to find the deepest point relative to the plane
        get_absolute_vertices(body_point, polygon_point, t2);
        int point_index = getSupportPoint(polygon_point, body_point->num_vertices, -n);
        s = dot(polygon_point[point_index], n) - dot(edge0, n);

        if (s > tol) {
          return false;
        } else if (s < -tol) {
          // find root
          float32 a, b, c;
          a = t1;
          b = t2;
          // root finding by bisection always approaching from positive side
          int b_iter = 0;
          while (1) {
            c = (a + b) / float32(2);
            // get plane determined earlier at new time t2
            edge0 = get_absolute_vertex(body_edge, feature_edge.index_1, c);
            edge1 = get_absolute_vertex(body_edge, feature_edge.index_2, c);
            edge = edge1 - edge0;
            n = cross(edge, edge0 - get_center(body_edge, c)) > float32(0) ? cross(float32(1), edge)
                                                                          : cross(edge, float32(1));
            n = normalize(n);
            // have to get all points of the polygon that doesn't make up the plane
            // in order to find the deepest point relative to the plane
            get_absolute_vertices(body_point, polygon_point, c);
            s = dot(polygon_point[point_index], n) - dot(edge0, n);
            // printf("[%d]: n [%f,%f] s %f, a %f, b %f, c %f\n", b_iter, n.x, n.y, s, a, b, c);
            if (abs(s) < tol) {  // root found
              break;
            } else if (s > float32(0)) {
              a = c;
            } else {
              b = c;
            }
            b_iter++;
            if (b_iter > 20) {
              printf("warning bisection root finding iterations exceeded");
              return false;
              // assert(false);
            }
          }
          t2 = c;
        } else {
          // projection of point on selected plane are within tolerance
          // check if they are actually close enough in full 2d
          t1 = t2;
          break;
        }
      }
    }
    get_absolute_vertices(body_a, polygon_a, t1);
    get_absolute_vertices(body_b, polygon_b, t1);

    // GJK algorithm returns 0 for distance if the shapes are overlapping
    // no matter how much they overlap by and the closest features are not accurate
    // s > -tol here so if we get polygon_distance of 0 we should be exiting, however,
    // we have no way of knowing if something went wrong so we can check by separating axis theorem instead
    // and then we can check if the minimum overlap magnitude is within tolerance and know for sure
    vec2 min_vector;
    float32 min_overlap;
    if (separating_axis_intersect(polygon_a, a_len, polygon_b, b_len, &min_vector, &min_overlap)) {
      // printf("min_overlap %f\n", min_overlap);
      if (min_overlap < tol) {
        *impact_time = t1;
        *impact = closest_a;
        *fa = feature_a;
        *fb = feature_b;
        return true;
      } else {
        // somehow we went too deep, should never hit this
        printf("warning went too deep\n");
        return false;
      }
    }

    // no collision at deepest point need to find new closest features
    distance =
        polygon_distance(polygon_a, a_len, polygon_b, b_len, NULL, NULL, &feature_a, &feature_b);
    if (distance == float32(0)) {
      // distance should not be zero because we would have caught any overlap with above SAT check
      assert(false);
    }
    iter++;
  }

  printf("warning maxed out overall bilateral advancement iterations\n");
  return false;
}

// 2D GJK, explanation: https://box2d.org/files/ErinCatto_GJK_GDC2010.pdf
// get closest distance between two polygons
// closest point on each polygon is returned through optional params closest_a and closest_b
float32 polygon_distance(const vec2* polygon_a, int len_a, const vec2* polygon_b, int len_b,
                        vec2* closest_a, vec2* closest_b, feature* feature_a, feature* feature_b) {
  simplex_vertex simplex[3];
  int simplex_size = 1;                       // number of simplex vertices
  vec2 origin{float32(0.0f), float32(0.0f)};  // origin is our target
  float32 divisor = float32(1.0f);

  // store indices of points on polygons that make up previous simplices so that
  // duplicates can be recognized
  int previous_index_a[3];
  int previous_index_b[3];
  int previous_simplex_size = 1;

  // choose starting point as first vertex arbitrarily
  simplex[0].b_coord = float32(1.0f);
  simplex[0].point_a = polygon_a[0];
  simplex[0].index_a = 0;

  simplex[0].point_b = polygon_b[0];
  simplex[0].index_b = 0;
  simplex[0].point = simplex[0].point_b - simplex[0].point_a;

  // iterate through GJK algorithm
  int iter = 0;
  while (iter < 20) {
    // save current simplex for future reference (just the active vertices)
    for (int i = 0; i < simplex_size; i++) {
      previous_index_a[i] = simplex[i].index_a;
      previous_index_b[i] = simplex[i].index_b;
    }
    previous_simplex_size = simplex_size;

    // remove unused vertices before continuing
    switch (simplex_size) {
      case 1:
        // impossible to have an unused vertex when there is just 1
        break;
      case 2:
        // reduce if possible
        simplex_size = solveSimplex2(simplex, &divisor, origin);
        break;
      case 3:
        // reduce if possible
        simplex_size = solveSimplex3(simplex, &divisor, origin);
        break;
    }

    // if we have still have 3 points after reducing then the origin must be inside of current simplex
    if (simplex_size == 3) {
      break;
    }

    vec2 d = getSearchDirection(simplex, simplex_size);
    if (dot(d, d) == 0) {
      break;
    }
    //compute new tentative simplex vertex using support points
    simplex[simplex_size].index_a = getSupportPoint(polygon_a, len_a, -d);
    simplex[simplex_size].point_a = polygon_a[simplex[simplex_size].index_a];

    simplex[simplex_size].index_b = getSupportPoint(polygon_b, len_b, d);
    simplex[simplex_size].point_b = polygon_b[simplex[simplex_size].index_b];

    simplex[simplex_size].point = simplex[simplex_size].point_b - simplex[simplex_size].point_a;

    iter++;

    // check for duplicate support points so that we know if we can terminate
    bool duplicate = false;
    for (int i = 0; i < previous_simplex_size; i++) {
      // if vertex for new support point that we just calculated has already been used
      if (simplex[simplex_size].index_a == previous_index_a[i] &&
          simplex[simplex_size].index_b == previous_index_b[i]) {
        duplicate = true;
        break;
      }
    }

    if (duplicate) {
      break;
    }

    simplex_size++;
  }

  vec2 a, b;
  float32 distance = getClosestPoints(simplex, simplex_size, divisor, &a, &b);
  if (closest_a) {
    *closest_a = a;
  }
  if (closest_b) {
    *closest_b = b;
  }

  feature fa = {};
  feature fb = {};
  if (simplex_size == 2) {
    if (simplex[0].index_b == simplex[1].index_b) {
      fb.index_1 = simplex[0].index_b;
      fb.edge = false;
      fa.index_1 = simplex[0].index_a;
      fa.index_2 = simplex[1].index_a;
      fa.edge = true;
    } else if (simplex[0].index_a == simplex[1].index_a) {
      fa.index_1 = simplex[0].index_a;
      fa.edge = false;
      fb.index_1 = simplex[0].index_b;
      fb.index_2 = simplex[1].index_b;
      fb.edge = true;
    } else {  // found an edge-aligned case, choose a point that is contained
      vec2 da01 = simplex[1].point_a - simplex[0].point_a;
      float32 da[2];
      da[0] = dot(simplex[0].point_a, da01);
      da[1] = dot(simplex[1].point_a, da01);
      // its gross to index something with the result of a conditional expression directly but I'm going to do it anyway hopefully it is clear
      int damax = da[1] > da[0];
      int damin = !damax;

      float32 db[2];
      db[0] = dot(simplex[0].point_b, da01);
      db[1] = dot(simplex[1].point_b, da01);
      int dbmax = db[1] > db[0];
      int dbmin = !dbmax;

      if (da[damin] < db[dbmin] && da[damax] > db[dbmax]) {
        // b is contained within A use either vertex of B as point and A as edge
        fb.index_1 = simplex[0].index_b;
        fb.edge = false;
        fa.index_1 = simplex[0].index_a;
        fa.index_2 = simplex[1].index_a;
        fa.edge = true;
      } else if (da[damin] > db[dbmin]) {
        // a min overlaps with b, use as min as point and B as edge
        fa.index_1 = simplex[damin].index_a;
        fa.edge = false;
        fb.index_1 = simplex[0].index_b;
        fb.index_2 = simplex[1].index_b;
        fb.edge = true;
      } else {
        // a max overlaps with b, use as max as point and B as edge
        fa.index_1 = simplex[damax].index_a;
        fa.edge = false;
        fb.index_1 = simplex[0].index_b;
        fb.index_2 = simplex[1].index_b;
        fb.edge = true;
      }
    }
  } else if (simplex_size == 1) {
    fa.index_1 = simplex[0].index_a;
    fa.edge = false;
    fb.index_1 = simplex[0].index_b;
    fb.edge = false;
  }

  if (feature_a) {
    *feature_a = fa;
  }
  if (feature_b) {
    *feature_b = fb;
  }

  return distance;
}

float32 getClosestPoints(simplex_vertex* simplex, int simplex_size, float32 divisor, vec2* a,
                         vec2* b) {
  switch (simplex_size) {
    case 1:
      *a = simplex[0].point_a;
      *b = simplex[0].point_b;
      break;
    case 2: {
      float32 s = float32(1.0f) / divisor;
      *a = (s * simplex[0].b_coord) * simplex[0].point_a +
           (s * simplex[1].b_coord) * simplex[1].point_a;
      *b = (s * simplex[0].b_coord) * simplex[0].point_b +
           (s * simplex[1].b_coord) * simplex[1].point_b;
    } break;
    case 3: {
      float32 s = float32(1.0f) / divisor;
      *a = (s * simplex[0].b_coord) * simplex[0].point_a +
           (s * simplex[1].b_coord) * simplex[1].point_a +
           (s * simplex[2].b_coord) * simplex[2].point_a;
      *b = *a;
    } break;
  }
  return distance(*a, *b);
}

// get support point for a polygon p with len # vertices for vector d. return vertex index
int getSupportPoint(const vec2* p, int len, vec2 d) {
  float32 farthest_value = dot(p[0], d);
  int farthest_index = 0;
  for (int i = 1; i < len; i++) {
    float32 value = dot(p[i], d);
    if (value > farthest_value) {
      farthest_value = value;
      farthest_index = i;
    }
  }
  return farthest_index;
}

// get search direction for a simplex, TODO: assumes that target is the origin (0,0)
vec2 getSearchDirection(simplex_vertex* simplex, int simplex_size) {
  switch (simplex_size) {
    case 1:
      // since target is origin (0,0) the vector of point inverted is the search direction
      return -(simplex[0].point);
    case 2: {
      // search direction for closest point on line segment is just normal of line segment towards the target
      // get normal of line segment using cross product
      vec2 AB = simplex[1].point - simplex[0].point;
      // make sure we are using normal positive towards search direction
      return cross(AB, -(simplex[0].point)) > float32(0) ? cross(float32(1), AB)
                                                         : cross(AB, float32(1));
    }
    default:
      // should never hit this
      return vec2(float32(0.0f), float32(0.0f));
  }
}

// closest point on line segment to target, returns updated number of simplex vertices
int solveSimplex2(simplex_vertex* simplex, float32* divisor, vec2 target) {
  // line segment between the two points on Minkowski difference
  vec2 A = simplex[0].point;
  vec2 B = simplex[1].point;

  // calculate barycentric coordinates
  // do not perform normalizing division yet to avoid potential divide by zero if line segment is 0 length
  float32 u = dot(target - B, A - B);
  float32 v = dot(target - A, B - A);

  // target is closest directly to vertex A
  if (v <= 0) {
    simplex[0].b_coord = float32(1.0f);
    *divisor = float32(1.0f);
    return 1;
  }
  // target is closest directly to vertex B
  if (u <= 0) {
    simplex[0] = simplex[1];
    simplex[0].b_coord = float32(1.0f);
    *divisor = float32(1.0f);
    return 1;
  }

  // target lies somewhere along the line segment AB
  simplex[0].b_coord = u;
  simplex[1].b_coord = v;
  // we can delay the division of barycentric coordinate further by just doing relative comparisons but
  // we need to save the divisor for the final distance output
  vec2 e = B - A;
  *divisor = dot(e, e);  // TODO: why?
  return 2;              // did not reduce simplex
}

// closest point on triangle to target, returns updated number of simplex vertices
int solveSimplex3(simplex_vertex* simplex, float32* divisor, vec2 target) {
  // triangle between the three points of Minkowski difference
  vec2 A = simplex[0].point;
  vec2 B = simplex[1].point;
  vec2 C = simplex[2].point;

  // calculate barycentric coordinates for line segments first and look at vertex regions
  float32 uAB = dot(target - B, A - B);
  float32 vAB = dot(target - A, B - A);

  float32 uBC = dot(target - C, B - C);
  float32 vBC = dot(target - B, C - B);

  float32 uCA = dot(target - A, C - A);
  float32 vCA = dot(target - C, A - C);

  // target is closest directly to vertex A
  if (vAB <= float32(0) && uCA <= float32(0)) {
    simplex[0].b_coord = 1;
    *divisor = 1;
    return 1;
  }
  // target is closest directly to vertex B
  if (uAB <= float32(0) && vBC <= float32(0)) {
    simplex[0] = simplex[1];
    simplex[0].b_coord = 1;
    *divisor = 1;
    return 1;
  }
  // target is closest directly to vertex C
  if (uBC <= float32(0) && vCA <= float32(0)) {
    simplex[0] = simplex[2];
    simplex[0].b_coord = 1;
    *divisor = 1;
    return 1;
  }

  // compute signed simplex area once
  float32 area = cross(B - A, C - A);
  // calculate barycentric coordinates for triangles
  float32 uABC = cross(B - target, C - target);
  float32 vABC = cross(C - target, A - target);
  float32 wABC = cross(A - target, B - target);

  // target is closest to line segment AB
  if (uAB > float32(0.0f) && vAB > float32(0.0f) && wABC * area <= float32(0.0f)) {
    simplex[0].b_coord = uAB;
    simplex[1].b_coord = vAB;
    vec2 e = B - A;
    *divisor = dot(e, e);
    return 2;
  }
  // target is closest to line segment BC
  if (uBC > float32(0.0f) && vBC > float32(0.0f) && uABC * area <= float32(0.0f)) {
    // eliminate current simplex vector A
    simplex[0] = simplex[1];
    simplex[1] = simplex[2];

    simplex[0].b_coord = uBC;
    simplex[1].b_coord = vBC;
    vec2 e = C - B;
    *divisor = dot(e, e);
    return 2;
  }
  // target is closes to line segment CA
  if (uCA > float32(0.0f) && vCA > float32(0.0f) && vABC * area <= float32(0.0f)) {
    // eliminate current simplex vector B
    simplex[1] = simplex[0];  // has to be line segment CA not AC for winding
    simplex[0] = simplex[2];

    simplex[0].b_coord = uCA;
    simplex[1].b_coord = vCA;
    vec2 e = A - C;
    *divisor = dot(e, e);
    return 2;
  }

  // target is inside of traingle ABC
  // The triangle area is guaranteed to be non-zero.
  simplex[0].b_coord = uABC;
  simplex[1].b_coord = vABC;
  simplex[2].b_coord = wABC;
  *divisor = area;
  return 3;
}

// https://en.wikipedia.org/wiki/Hyperplane_separation_theorem#Use_in_collision_detection
bool separating_axis_intersect(const vec2 a[], int a_len, const vec2 b[], int b_len,
                             vec2* minimum_vector, float32* minimum_overlap) {
  float32 proj_min_a;
  float32 proj_max_a;
  float32 proj_min_b;
  float32 proj_max_b;
  float32 min_overlap = F32_MAX;
  vec2 min_vector;
  vec2 axis;

  // check overlap for faces of shape A and shape B
  for (int i = 0; i < a_len + b_len; i++) {
    if (i < a_len) {
      axis = a[(i + 1) % a_len] - a[i];
      axis = normalize(axis);
    } else {
      int index = i - a_len;
      axis = b[(index + 1) % b_len] - b[index];
      axis = normalize(axis);
    }

    // check all vertices of a
    proj_min_a = proj_max_a = dot(axis, a[0]);  // set initial min/max values
    for (int j = 1; j < a_len; j++) {
      float32 p = dot(axis, a[j]);
      if (p < proj_min_a) {
        proj_min_a = p;
      } else if (p > proj_max_a) {
        proj_max_a = p;
      }
    }

    // check all vertices of b
    proj_min_b = proj_max_b = dot(axis, b[0]);  // set initial first min/max values
    for (int j = 1; j < b_len; j++) {
      float32 p = dot(axis, b[j]);
      if (p < proj_min_b) {
        proj_min_b = p;
      } else if (p > proj_max_b) {
        proj_max_b = p;
      }
    }

    // calculate overlap
    float32 overlap = min(proj_max_a, proj_max_b) - max(proj_min_a, proj_min_b);

    // check for total containment
    if (((proj_max_a > proj_max_b) && (proj_min_a < proj_min_b)) ||
        ((proj_max_b > proj_max_a) && (proj_min_b < proj_min_a))) {
      // add overlap to account for containment
      float32 dmin = abs(proj_min_a - proj_min_b);
      float32 dmax = abs(proj_max_a - proj_max_b);
      if (dmin < dmax) {
        overlap += dmin;
      } else {
        overlap += dmax;
      }
    }

    if (overlap < -tol) {
      return false;
    } else {
      if (overlap < min_overlap) {
        min_overlap = overlap;
        min_vector = axis;
        // vector in terms of direction to move polygon A
        if ((proj_max_b - proj_min_a) > (proj_max_a - proj_min_b)) {
          min_vector = -min_vector;
        }
      }
    }
  }

  if (minimum_overlap)
    *minimum_overlap = min_overlap;
  if (minimum_vector)
    *minimum_vector = min_vector;
  return true;
}

/*points a0,a1, b0,b1 find intersection of line segments defined by these points.
  va is vector a0->a1, vb is vector b0->b1
  system of equations parameterized s,t [0:1]
  expression f(s,t) => a0 + va*s = b0 + vb*t
                    => va*s - vb*t = b0 - a0
  split into x and y to get 2 independent equations
  fx(s,t) => va.x*s - vb.x*t = b0.x - a0.x
  fy(s,t) => va.y*s - vb.y*t = b0.y - a0.y
  in matrix form for Cramer's rule:
  |va.x -vb.x| |s| = |ab.x|
  |va.y -vb.y| |t|   |ab.y|

  s =    |ab.x -vb.x|       t =    |va.x ab.x|
      det|ab.y -vb.y|           det|va.y ab.y|
      ________________          ________________
          |va.x -vb.x|              |va.x -vb.x|
      det|va.y -vb.y|           det|va.y -vb.y|
  
  s = ( ab.x*(-vb.y) - ab.y*(-vb.x) ) /  ( va.x*(-vb.y) - va.y*(-vb.x) )
  t = ( va.x*ab.y - va.y*ab.x ) /  ( va.x*(-vb.y) - va.y*(-vb.x) )
  we can pull out the denominator from this to avoid divisions by zero and do an early check
  d = ( va.x*(-vb.y) - va.y*(-vb.x) )

  s = ( ab.x*(-vb.y) - ab.y*(-vb.x) ) /  d
  t = ( va.x*ab.y - va.y*ab.x ) /  d
*/
bool line_segment_intersect(vec2 a0, vec2 a1, vec2 b0, vec2 b1, vec2* intersection, float32* ta,
                          float32* tb) {
  float32 s, t, d;
  vec2 va = {a1.x - a0.x, a1.y - a0.y};
  vec2 vb = {b1.x - b0.x, b1.y - b0.y};
  vec2 ab = {b0.x - a0.x, b0.y - a0.y};

  d = va.x * (-vb.y) - va.y * (-vb.x);
  if (d == float32(0)) {
    return false;
  }

  s = (ab.x * (-vb.y) - ab.y * (-vb.x)) / d;
  t = (va.x * ab.y - va.y * ab.x) / d;
  if (s < float32(0) || s > float32(1) || t < float32(0) || t > float32(1)) {
    return false;
  }

  if (intersection) {
    *intersection = {a0.x + (t * va.x), a0.y + (t * va.y)};
  }
  if (ta) {
    *ta = s;  // normalized point along line a where intersection occurred
  }
  if (tb) {
    *tb = t;  // normalized point along line b where intersection occurred
  }

  return true;
}

// get vertices translated to center with angle r
// t is return parameter vec2* with length of at least num_vertices
void get_absolute_vertices(const body* b, vec2* v) {
  mat22 rot;  // rotation matrix
  rot.set(b->r);
  for (int i = 0; i < b->num_vertices; i++) {
    v[i] = mul(rot, b->vertices[i]);
    v[i] += b->center;
  }
}

// get absolute vertices when applying velocity timestep t
void get_absolute_vertices(const body* b, vec2* v, float32 t) {
  mat22 rot;  // rotation matrix
  rot.set(b->r + (t * b->w));
  for (int i = 0; i < b->num_vertices; i++) {
    v[i] = mul(rot, b->vertices[i]);
    v[i] += b->center + (t * b->vel);
  }
}

// get absolute vertices when applying velocity timestep t
vec2 get_absolute_vertex(const body* b, int index, float32 t) {
  vec2 v;
  mat22 rot;  // rotation matrix
  rot.set(b->r + (t * b->w));
  v = mul(rot, b->vertices[index]);
  v += b->center + (t * b->vel);
  return v;
}

// get absolute vertices when applying velocity timestep t
vec2 get_absolute_vertex(const body* b, int index) {
  vec2 v;
  mat22 rot;  // rotation matrix
  rot.set(b->r + b->w);
  v = mul(rot, b->vertices[index]);
  v += b->center + b->vel;
  return v;
}

vec2 get_center(const body* b, float32 t) {
  return b->center + (t * b->vel);
}