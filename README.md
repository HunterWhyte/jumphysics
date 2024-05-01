### Investigation into 2D Collision Detection for Convex Polygons
**The goal**: Detect collisions between 2 moving (rotating and translating) N-sided convex polygons. \
Our simulation runs at a timestep wherein each timestep polygons are moved based on their velocity and angular velocity. The simplest approach is to perform collision detection at each of these discrete timesteps.

## Discrete Collision Detection
Performing collision detection on discrete timesteps means that each timestep the polygons are moved then checked if they are overlapping. The algorithm used to determine if two convex polygons are overlapping is the ***separating axis theorem***. Which states:
`Two closed convex objects are disjoint if there exists a line ("separating axis") onto which the two objects' projections are disjoint.` [0]

Meaning that if we have two convex polygons A and B that are overlapping then the projections of A and B onto *any* axis will also be overlapping. If the polygons are not overlapping on any given axis then they are disjoint.

`image of two polygons showing the separating axis`


If we first consider this in terms of a point intersecting a polygon. To do this we pick an axis, project the polygon onto the axis and check if the point projected onto the axis is within the bounds of the polygon - if the point is outside of the polygon for *any* axis then we know that it is not within the polygon. It turns out that we don't have to check *every* possible axis though (or else this would be useless) - we can each face of the polygon as the feature direction for the separating axis.

`demo of having mouse pointer intersect a polygon, displaying separating axis when outside`

## Continuous Collision Detection
Where discrete collision detection fails. 

Challenges in continuous collision detection. 

### **Computing Distance**
### Point to Line Segment
guh
### Point to Triangle
guh
### Point to Convex Polygon
guh
### Convex Polygon to Convex Polygon
guh


### **Sources**:
- [0] https://en.wikipedia.org/wiki/Hyperplane_separation_theorem#Use_in_collision_detection
- [1] https://box2d.org/files/ErinCatto_ContinuousCollision_GDC2013.pdf
- [2] https://en.wikipedia.org/wiki/Minkowski_addition
- [3] https://caseymuratori.com/blog_0003
- [4] https://box2d.org/files/ErinCatto_GJK_GDC2010.pdf
