#include <stdio.h>
#include <raylib.h>
#include <raymath.h>		/* includes math.h */
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define PROFILE_IMPLEMENTATION
#include "../lib/profile.h"

#define SW 1920
#define SH 1080
#define PHYSICS_IMPLEMENTATION
#include "physics.h"
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
Vector2 _t;
#define CX SW/2
#define CY SH/2
int zoom = 1;
#define T(v) (_t = v, (Vector2) {CX + _t.x*zoom, CY - _t.y*zoom})
#define _T(v) (_t = v, (Vector2) {(-CX + _t.x)/zoom, (CY - _t.y)/zoom})

const float CR = (SW > SH ? SH : SW) / 2;
#define G  10			/* Gravity coefficient */
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
/* TODO: forces of type v.o[i].a += forces[j](w, v.o[i]) */
Vector2 gravity(object *a, object *b)
{
	/* printf("flags: %d %d\n", a->flags, b->flags); */
	if (a->flags&PHY_GRAVITY && !(b->flags&PHY_MASSLESS)) {
		Vector2 r = Vector2Subtract(b->p, a->p);
		float l = Vector2Length(r);
		return Vector2Scale(r, powf(b->r, 3) * G/(l*l*l + 1));
	} else if (a->flags&PHY_UNIVERSAL_GRAVITY_RECIEVER && b->flags&PHY_UNIVERSAL_GRAVITY_DONOR) {
		return (Vector2) {0, G};
	} else {
		return Vector2Zero();
	}
}

#define SAVE_COUNT 256
#define HIST_SIZE 4096
Vector2 position_history[SAVE_COUNT][HIST_SIZE] = {0};
int pointer = 0;
void reset(world *w, float delta)
{
	w->o.size = 0;
	pointer = 0;
	world_add_object_at(w, ((Vector2) {SW/2, SH/2}), PHY_GRAVITY);
	AT(w->o, 0).r = 10;
	/* world_add_object_at(w, (Vector2) {SW/2, SH/2}, PHY_INVISIBLE); */
	/* world_add_object_at(w, (Vector2) {SW/2, SH/2}, PHY_GRAVITY); */
	/* world_add_object_at(w, Vector2Zero(), PHY_UNIVERSAL_GRAVITY_DONOR | PHY_INVISIBLE); */
	for (int i = 0; i < SAVE_COUNT; i++) {
		for (int j = 0; j < HIST_SIZE; j++) {
			position_history[i][j] = Vector2Zero();
		}
	}
}

void object_log(int idx, object o);
void object_show(object o);
double profile_time[16];

int main()
{
	world w = {0};
	reset(&w, 0);
	world_add_force(&w, gravity);
	bool paused = false;
	bool hud = true;
	Vector2 setpos;
	srandom(time(NULL));

	InitWindow(SW, SH, "Physics Simulation");
	SetTargetFPS(60);
	ToggleFullscreen();
	ToggleFullscreen();
	int time_count, time_max;
	while (!WindowShouldClose())
	{
		time_count++;
		profile_time[time_count++] = profile();
		float delta = GetFrameTime();
		if (delta > 0.01) delta = 0.01;

		/* Handle Keys */
		if (KEY(R)) reset(&w, delta);
		if (KEY(A)) {
			world_add_object_at(&w, GetMousePosition(), PHY_GRAVITY);
			AT(w.o, w.o.size - 1).r = random()%7 + 3;
		}
		if (KEY(SPACE)) paused = !paused;
		if (KEY(S)) world_step(&w, delta), paused = true;
		if (KEY(D) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) setpos = GetMousePosition();
		if (KEY(BACKSLASH)) hud = !hud;
		if (IsKeyReleased(KEY_D) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			world_add_object_at(&w, setpos, PHY_GRAVITY);
			/* pp = p - d v */
			AT(w.o, w.o.size - 1).pp = Vector2Subtract(AT(w.o, w.o.size - 1).p,
								Vector2Scale(Vector2Subtract(GetMousePosition(), setpos), delta));
			AT(w.o, w.o.size - 1).r = random()%7 + 3;
		}
		if (KEY(F)) {
			float r = 100;
			int mindx = -1;
			for (int i = 0; i < w.o.size; i++) {
				float l = Vector2Distance(GetMousePosition(), AT(w.o, i).p);
				if (l < r) {
					r = l;
					mindx = i;
				}
			}
			if (mindx >= 0) object_log(mindx, AT(w.o, mindx));
		}
		if (KEY(L))
			for (int i = 0; i < w.o.size; i++)
				if (!(AT(w.o, i).flags&PHY_INVISIBLE))
					object_log(i, AT(w.o, i));
		if (KEY(T)) {
			for (int i = 0; i < 5; i++)
				printf("%f ", profile_time[i]);
			printf("\n");
		}

		/* Apply forces */
		profile_time[time_count++] = profile();
		world_apply_forces(&w);


		/* Update positions */
		profile_time[time_count++] = profile();
		if (!paused) {
			for (int i = 0; i < MIN(SAVE_COUNT, w.o.size); i++)
				if (w.o.size)
					position_history[i][pointer] = w.o.list[i].p;
			pointer = (pointer+1)%HIST_SIZE;
			world_step(&w, delta);
		}


		/* Vector2 com = Vector2Zero(); */
		/* for (int i = 0; i < w.o.size; i++) { */
			/* com = Vector2Add(AT(w.o, i).p, com); */
		/* } */
		/* com = Vector2Scale(com, 1./w.o.size); */
		/* world_center(&w, com); */
		/* world_center(&w, AT(w.o, 0).p); */

		/* Draw */
		profile_time[time_count++] = profile();
		BeginDrawing();
		ClearBackground(BLACK);
		if (KEY(R)) ClearBackground(BLACK);
		if (hud) {
			DrawFPS(0, 0);
		}
		for (int i = 0; i < MIN(SAVE_COUNT, w.o.size); i++) {
			for (int j = 0; j < HIST_SIZE; j++) {
				int idx = (pointer + HIST_SIZE + j)%HIST_SIZE;
				DrawPixelV(position_history[i][idx], (Color) {255, 255, 255, 255.*j/HIST_SIZE});
			}
		}
		for (int i = 0; i < w.o.size; i++)
			object_show(AT(w.o, i));
		EndDrawing();
		profile_time[time_count++] = profile();
		if (time_count > time_max) time_max = time_count;
	}
	CloseWindow();
	world_free(&w);

	return 0;
}

void object_log(int idx, object o)
{
	Vector2 p = o.p, v = o.v, a = o.a;
	printf("object %d: (%.3f %.3f) (%.3f %.3f) (%.3f %.3f) %08b\n", idx, p.x, p.y, v.x, v.y, a.x, a.y, o.flags);
}

void object_show(object o)
{
	if (!(o.flags&PHY_INVISIBLE)) {
		/* Vector2 v = Vector2Scale(object_velocity(&(AT(o, i)), delta), 0.1); */
		/* DrawLineV(Vector2Add(AT(o, i).p, v), Vector2Add( */
		/* 		Vector2Add(AT(o, i).p, v), */
		/* 		Vector2Scale(AT(o, i).a, 0.1)), WHITE); */
		/* DrawLineV(AT(o, i).p, Vector2Add(AT(o, i).p, v), RED); */
		/* DrawCircleV(AT(o, i).p, 3, (Color) {138*i, 244, 138, 255}); */
		/* DrawPixelV(o.p, (Color) {255. * o.mass / 1000., 34, 56, 255}); */
		DrawCircleV(o.p, o.r, (Color) {255. * o.r / 10., 34, 56, 255});
	}
}
