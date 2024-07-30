#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <raylib.h>
#include <raymath.h>
#include <math.h>

#define SW 500
#define SH 500
#define WORLD_INITIAL_CAP 64
#define PHYSICS_IMPLEMENTATION
#include "physics.h"

const float CR = (SW > SH ? SH : SW) / 2;
#define G  100			/* Gravity coefficient */
#define KEY(k) (IsKeyPressed(KEY_##k) || IsKeyPressedRepeat(KEY_##k))

enum flags {
	PHY_INVISIBLE = 0x1,
	PHY_GRAVITY = 0x2,
	PHY_MASSLESS = 0x4,
	PHY_UNIVERSAL_GRAVITY_RECIEVER = 0x8,
	PHY_UNIVERSAL_GRAVITY_DONOR = 0x10,
};

/* The interface to choosing which objects to apply the force to is
 * done by flags */
/* forces of type v.o[i].a += forces[j](w, v.o[i]) */
Vector2 gravity(object *a, object *b)
{
	/* printf("flags: %d %d\n", a->flags, b->flags); */
	if (a->flags&PHY_GRAVITY && !(b->flags&PHY_MASSLESS)) {
		Vector2 r = Vector2Subtract(b->p, a->p);
		float l = Vector2Length(r);
		return Vector2Scale(r, G/(l*l*l + 1));
	} else if (a->flags&PHY_UNIVERSAL_GRAVITY_RECIEVER && b->flags&PHY_UNIVERSAL_GRAVITY_DONOR) {
		return (Vector2) {0, G};
	} else {
		return Vector2Zero();
	}
}

void reset(world *w)
{
	w->o_size = 0;
	/* world_add_object_at(w, (Vector2) {SW/2, SH/2}, PHY_INVISIBLE); */
	/* world_add_object_at(w, (Vector2) {SW/2, SH/2}, PHY_GRAVITY); */
	world_add_object_at(w, Vector2Zero(), PHY_UNIVERSAL_GRAVITY_DONOR | PHY_INVISIBLE);
}

void object_log(int idx, object *o)
{
	Vector2 p = o->p, v = o->v, a = o->a;
	printf("object %d: (%.3f %.3f) (%.3f %.3f) (%.3f %.3f) %08b\n", idx, p.x, p.y, v.x, v.y, a.x, a.y, o->flags);
}

int main()
{
	world w = {0};
	reset(&w);
	world_add_force(&w, gravity);
	bool paused = false;
	Vector2 setpos;

	InitWindow(SW, SH, "Physics Simulation");
	SetTargetFPS(60);
	ToggleFullscreen();
	ToggleFullscreen();
	while (!WindowShouldClose())
	{
		float delta = GetFrameTime();
		if (delta > 0.01) delta = 0.01;

		/* Handle Keys */
		if (KEY(R)) reset(&w);
		if (KEY(A)) world_add_object_at(&w, GetMousePosition(), PHY_UNIVERSAL_GRAVITY_RECIEVER);
		if (KEY(SPACE)) paused = !paused;
		if (KEY(S)) world_step(&w, delta), paused = true;
		if (KEY(D) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) setpos = GetMousePosition();
		if (IsKeyReleased(KEY_D) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			world_add_object_at(&w, setpos, PHY_UNIVERSAL_GRAVITY_RECIEVER);
			w.objects[w.o_size - 1].pp = Vector2Subtract(w.objects[w.o_size - 1].p,
								Vector2Scale(Vector2Subtract(GetMousePosition(), setpos), delta));
		}
		if (KEY(F)) {
			float r = 100;
			int mindx = -1;
			for (int i = 0; i < w.o_size; i++) {
				float l = Vector2Distance(GetMousePosition(), w.objects[i].p);
				if (l < r) {
					r = l;
					mindx = i;
				}
			}
			if (mindx >= 0) object_log(mindx, &(w.objects[mindx]));
		}
		if (KEY(L))
			for (int i = 0; i < w.o_size; i++)
				if (!(w.objects[i].flags&PHY_INVISIBLE))
					object_log(i, &(w.objects[i]));

		/* Apply forces */
		world_apply_forces(&w);

		/* Update positions */
		if (!paused) world_step(&w, delta);

		/* Vector2 com = Vector2Zero(); */
		/* for (int i = 0; i < w.o_size; i++) { */
			/* com = Vector2Add(w.objects[i].p, com); */
		/* } */
		/* com = Vector2Scale(com, 1./w.o_size); */
		/* world_center(&w, com); */
		/* world_shift(&w, w.objects[0].p)); */

		/* Draw */
		BeginDrawing();
		if (KEY(R)) ClearBackground(BLACK);
		/* ClearBackground(BLACK); */
		/* DrawFPS(0, 0); */
		for (int i = 0; i < w.o_size; i++) {
			if (!(w.objects[i].flags&PHY_INVISIBLE)) {
				/* Vector2 v = Vector2Scale(object_velocity(&(w.objects[i]), delta), 0.1); */
				/* DrawLineV(Vector2Add(w.objects[i].p, v), Vector2Add( */
				/* 		Vector2Add(w.objects[i].p, v), */
				/* 		Vector2Scale(w.objects[i].a, 0.1)), WHITE); */
				/* DrawLineV(w.objects[i].p, Vector2Add(w.objects[i].p, v), RED); */
				/* DrawCircleV(w.objects[i].p, 3, (Color) {138*i, 244, 138, 255}); */
				DrawPixelV(w.objects[i].p, (Color) {138*i, 244, 138, 255});
			}
		}
		EndDrawing();
		errno = 0;
	}
	CloseWindow();
	if (w.o_capacity) free(w.objects);

	return 0;
}
