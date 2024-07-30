#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdlib.h>

typedef struct object {
	Vector2 p, pp;	/* Verlet */
	Vector2 v, a;
	unsigned int flags;
} object;
Vector2 object_velocity(object *o, float delta);

typedef Vector2 (*force)(object*, object*);
typedef struct world {
	object *objects;
	int o_size, o_capacity;
	force *forces;
	int f_size, f_capacity;
} world;
void world_add_object_at(world *w, Vector2 p, unsigned int flags);
void world_add_force(world *w, force f);
void world_apply_forces(world *w);
void world_step(world *w, float delta);
void world_center(world *w, Vector2 origin);

#endif /* PHYSICS_H */

#ifdef PHYSICS_IMPLEMENTATION

void world_add_object_at(world *w, Vector2 p, unsigned int flags)
{
	/* array_append(w->objects, (object) {p, p, Vector2Zero(), flags}) */
	if (w->o_capacity == 0) {
		w->objects = calloc(WORLD_INITIAL_CAP, sizeof (object));
		w->o_capacity = WORLD_INITIAL_CAP;
	} else if (w->o_capacity < w->o_size) {
		perror("Unreachable");
	} else if (w->o_capacity == w->o_size) {
		w->objects = reallocarray(w->objects, w->o_capacity *= 2, sizeof (object));
	}
	if (w->objects == NULL) {
		perror("Allocation");
		return;
	}
	w->objects[w->o_size++] = (object) {p, p, Vector2Zero(), Vector2Zero(), flags};
}

Vector2 object_velocity(object *o, float delta)
{
	return Vector2Scale(
		Vector2Subtract(Vector2Subtract(o->p, o->pp),
				Vector2Scale(o->a, delta*delta)),
		1./delta);
}

void world_add_force(world *w, force f)
{
	/* array_append(w->forces, force) */
	if (w->f_capacity == 0) {
		w->forces = calloc(WORLD_INITIAL_CAP, sizeof (force));
		w->f_capacity = WORLD_INITIAL_CAP;
	} else if (w->f_capacity < w->f_size) {
		perror("Unreachable");
	} else if (w->f_capacity == w->f_size) {
		w->forces = reallocarray(w->forces, w->f_capacity *= 2, sizeof (force));
	}
	if (w->forces == NULL) {
		perror("Allocation");
		return;
	}
	w->forces[w->f_size++] = f;
}

void world_apply_forces(world *w)
{
	if (w->f_size == 0 || w->o_size < 2) return;
	for (int i = 0; i < w->o_size; i++) {
		w->objects[i].a = Vector2Zero();
		for (int j = 0; j < w->o_size; j++) {
			if (i==j) continue;
			for (int k = 0; k < w->f_size; k++) {
				Vector2 acc = w->forces[k](&w->objects[i], &w->objects[j]);
				w->objects[i].a = Vector2Add(w->objects[i].a, acc);
			}
		}
	}
}

void world_step(world *w, float delta)
{
	for (object *ptr = w->objects, *end = w->objects + w->o_size; ptr != end; ptr++) {
		Vector2 tmp = ptr->p;
		/* p = 2p - pp + δa */
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
	for (int i = 0; i < w->o_size; i++) {
		w->objects[i].p = Vector2Add(w->objects[i].p, shift);
		w->objects[i].pp = Vector2Add(w->objects[i].pp, shift);
	}
}

#endif /* PHYSICS_IMPLEMENTATION */

