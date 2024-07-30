#ifndef PHYSICS_H
#define PHYSICS_H

#include <raymath.h>
#define ARRAY_IMPLEMENTATION
#include "array.h"

typedef struct object {
	Vector2 p, pp;	/* Verlet */
	int mass;
	Vector2 v, a;
	unsigned int flags;
} object;
Vector2 object_velocity(object *o, float delta);

#ifndef WORLD_INITIAL_CAP
#define WORLD_INITIAL_CAP 64
#endif
typedef Vector2 (*force)(object*, object*);
typedef struct world {
	ARRAY(object) o;
	ARRAY(force) f;
	/* object *objects; */
	/* int o_size, o_capacity; */
	/* force *forces; */
	/* int f_size, f_capacity; */
} world;
#define world_add_object_at(w, p, flags) array_append((w)->o, ((object) {(p), (p), 0, Vector2Zero(), Vector2Zero(), flags}))
#define world_add_force(w, F) array_append((w)->f, F)
void world_apply_forces(world *w);
void world_step(world *w, float delta);
void world_center(world *w, Vector2 origin);
#define world_free(w)			\
	do {					\
		array_free((w)->o);		\
		array_free((w)->f);		\
	} while(0)

#endif /* PHYSICS_H */

#ifdef PHYSICS_IMPLEMENTATION

Vector2 object_velocity(object *o, float delta)
{
	/* (p - pp - a δ²) / δ */
	return Vector2Scale(
		Vector2Subtract(Vector2Subtract(o->p, o->pp),
				Vector2Scale(o->a, delta*delta)),
		1./delta);
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

void world_step(world *w, float delta)
{
	for (object *ptr = w->o.list, *end = w->o.list + w->o.size; ptr != end; ptr++) {
		Vector2 tmp = ptr->p;
		/* p = 2 p - pp + a δ² */
		ptr->p = Vector2Add(
			Vector2Subtract(Vector2Scale(ptr->p, 2), ptr->pp),
			Vector2Scale(ptr->a, delta*delta));
		ptr->pp = tmp;
		ptr->v = object_velocity(ptr, delta);
	}
}

void world_center(world *w, Vector2 origin)
{
	Vector2 shift = Vector2Subtract((Vector2) {SW/2, SH/2}, origin);
	for (int i = 0; i < w->o.size; i++) {
		AT(w->o, i).p = Vector2Add(AT(w->o, i).p, shift);
		AT(w->o, i).pp = Vector2Add(AT(w->o, i).pp, shift);
	}
}

#endif /* PHYSICS_IMPLEMENTATION */

