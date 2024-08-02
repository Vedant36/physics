#ifndef PHYSICS_H
#define PHYSICS_H

#include <raymath.h>
#define ARRAY_IMPLEMENTATION
#include "array.h"

#define A(v, w) Vector2Add(v, w)
#define S(v, w) Vector2Subtract(v, w)
#define C(v, s) Vector2Scale(v, s)
#define L(v) Vector2Length(v)

typedef struct frame {
	Vector2 p;
} frame;

typedef struct object {
	Vector2 p, pp;	/* Verlet */
	float r;
	unsigned int flags;

	Vector2 v, a;
} object;

#ifndef WORLD_INITIAL_CAP
#define WORLD_INITIAL_CAP 64
#endif
typedef Vector2 (*force)(object*, object*);
typedef struct world {
	ARRAY(object) o;
	ARRAY(force) f;
} world;
int world_add_object(world *w, Vector2 p, Vector2 v, float r, unsigned int flags, float delta);
#define world_add_object_at(w, p, flags) world_add_object(w, p, Vector2Zero(), 0, flags, 0)
#define world_add_force(w, F) array_append((w)->f, F)
void world_apply_forces(world *w);
void world_step(world *w, double delta);
#define world_free(w)			\
	do {					\
		array_free((w)->o);		\
		array_free((w)->f);		\
	} while(0)

#endif /* PHYSICS_H */

#ifdef PHYSICS_IMPLEMENTATION

inline int world_add_object(world *w, Vector2 p, Vector2 v, float r, unsigned int flags, float delta)
{
	array_append(w->o, ((object) {p, S(p, C(v, delta)), r, flags, v, Vector2Zero()}));
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
				force f = AT(w->f, k);
				object *a = &AT(w->o, i);
				object *b = &AT(w->o, j);
				Vector2 acc = f(a, b);
				AT(w->o, i).a = A(a->a, acc);
			}
		}
	}
}

void world_step(world *w, double delta)
{
	for (int i = 0; i < w->o.size; i++) {
		object *ptr = &AT(w->o, i);
		Vector2 p = ptr->p, pp = ptr->pp, v = ptr->v, a = ptr->a;
		/* ptr->v = A(v, C(a, delta)); */
		/* ptr->p = A(p, C(v, delta)); */

		/* p = 2 p - pp + a δ² */
		ptr->p.x = 2*p.x - pp.x + a.x * delta * delta;
		ptr->p.y = 2*p.y - pp.y + a.y * delta * delta;
		ptr->v = A(v, C(a, delta));
		ptr->pp = p;
	}
}

#endif /* PHYSICS_IMPLEMENTATION */

