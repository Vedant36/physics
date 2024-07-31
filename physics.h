#ifndef PHYSICS_H
#define PHYSICS_H

#include <raylib.h>
#include <raymath.h>
#define ARRAY_IMPLEMENTATION
#include "array.h"

#define A(v, w) Vector2Add(v, w)
#define S(v, w) Vector2Subtract(v, w)
#define C(v, s) Vector2Scale(v, s)
#define L(v) Vector2Length(v)

typedef struct object {
	Vector2 p, pp;	/* Verlet */
	unsigned int r, flags;

	Vector2 v, a;
} object;
void *object_set_velocity;

#ifndef WORLD_INITIAL_CAP
#define WORLD_INITIAL_CAP 64
#endif
typedef Vector2 (*force)(object*, object*);
typedef struct world {
	ARRAY(object) o;
	ARRAY(force) f;
} world;
int world_add_object(world *w, Vector2 p, Vector2 v, unsigned int r, unsigned int flags, float delta);
#define world_add_object_at(w, p, flags) world_add_object(w, p, Vector2Zero(), 0, flags, 0)
#define world_add_force(w, F) array_append((w)->f, F)
void world_apply_forces(world *w);
void world_step(world *w, double delta);
void world_center(world *w, Vector2 origin);
#define world_free(w)				\
	do {					\
		array_free((w)->o);		\
		array_free((w)->f);		\
	} while(0)

#endif /* PHYSICS_H */

#ifdef PHYSICS_IMPLEMENTATION

inline int world_add_object(world *w, Vector2 p, Vector2 v, unsigned int r, unsigned int flags, float delta)
{
	array_append(w->o, ((object) {p, S(p, C(v, delta)), r, flags, v, Vector2Zero()}));
	Vector2 pp = AT(w->o, w->o.size - 1).pp;
	printf("(%f %f) (%f %f) (%f %f)\n", p.x, p.y, pp.x, pp.y, v.x, v.y);
	return w->o.size - 1;
}
void world_apply_forces(world *w)
{
	if (w->f.size == 0 || w->o.size < 2) return;
	for (int i = 0; i < w->o.size; i++) {
		AT(w->o, i).a = Vector2Zero();
		for (int j = 0; j < w->o.size; j++) {
			if (i==j) continue;
			for (int k = 0; k < w->f.size; k++) {
				Vector2 acc = AT(w->f, k)(&AT(w->o, i), &AT(w->o, j));
				AT(w->o, i).a = Vector2Add(AT(w->o, i).a, acc);
			}
		}
	}
}

void world_step(world *w, double delta)
{
	for (int i = 0; i < w->o.size; i++) {
		object *ptr = &AT(w->o, i);
	/* } */
	/* for (object *ptr = w->o.list, *end = w->o.list + w->o.size; ptr != end; ptr++) { */
		Vector2 p = ptr->p, pp = ptr->pp, v = ptr->v, a = ptr->a;
		ptr->p.x = p.x + v.x * delta;
		ptr->p.y = p.y + v.y * delta;
		ptr->v.x = v.x + a.x * delta;
		ptr->v.y = v.y + a.y * delta;
		printf("%f %f %f\n", delta, delta*delta, delta*delta*delta);

		// p' = 2 p - pp + a δ² + O(δ⁴)
		/* ptr->p.x = 2*p.x - pp.x + delta*delta*a.x; */
		/* ptr->p.y = 2*p.y - pp.y + delta*delta*a.y; */
		/* ptr->p = A( S( C(p, 2.), pp ), C(a, delta*delta) ); */
		// v' = 3/2δ (x' - 'x) - 2 v + δ a + O(δ³)
		/* ptr->v = A(C(S(ptr->p, pp), 3./2/delta), A(C(v, -2.), C(a, delta))); */
		// v' = v + δ a
		/* ptr->v = A(v, C(a, delta)); */
		ptr->pp = p;
	}
}

void world_center(world *w, Vector2 origin)
{
	Vector2 shift = Vector2Subtract((Vector2) {SW/2, SH/2}, origin);
	for (int i = 0; i < w->o.size; i++) {
		AT(w->o, i).p = A(AT(w->o, i).p, shift);
		AT(w->o, i).pp = A(AT(w->o, i).pp, shift);
	}
}

#endif /* PHYSICS_IMPLEMENTATION */

