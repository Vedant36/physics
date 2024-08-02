/* TODO:
 * - forces of type v.o[i].a += forces[j](w, v.o[i])
 * - symplectic integration to reduce energy deviation
 */
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>		/* includes math.h */
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define PROFILE_IMPLEMENTATION
#include "profile.h"
#define profile() 0		/* uncomment if you don't want to use profile.h */

#define SW 500
#define SH 500
#define PHYSICS_IMPLEMENTATION
#include "physics.h"

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
Vector2 _t;
Vector2 origin = {SW/2, SH/2};
int zoom = 1;
#define T(v) (_t = v, (Vector2) {origin.x + _t.x*zoom, origin.y - _t.y*zoom})
#define _T(v) (_t = v, (Vector2) {(-origin.x + _t.x)/zoom, (origin.y - _t.y)/zoom})
const float CR = (SW > SH ? SH : SW) / 2;
#define G  10			/* Gravity coefficient */
#define SUBFRAMES 10.
#define KEY(k) (IsKeyPressed(KEY_##k) || IsKeyPressedRepeat(KEY_##k))
#define BIND(key, param) if (KEY(key)) param += 1 - 2 * IsKeyDown(KEY_LEFT_SHIFT)

enum flags {
	PHY_INVISIBLE = 0x1,
	PHY_GRAVITY = 0x2,
	PHY_MASSLESS = 0x4,
	PHY_UNIVERSAL_GRAVITY_RECIEVER = 0x8,
	PHY_UNIVERSAL_GRAVITY_DONOR = 0x10,
};

/* The interface to choosing which objects to apply the force to is
 * done by flags */
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
int pointer = 0, speed;
bool paused, hud, stepped = false;
void reset(world *w, double delta)
{
	w->o.size = 0;
	hud = true;
	paused = true;
	pointer = 0;
	speed = 1;
	world_add_object_at(w, Vector2Zero(), PHY_GRAVITY);
	AT(w->o, 0).r = 100;
	world_add_object_at(w, ((Vector2) {0, -150}), PHY_GRAVITY);
	Vector2 v = {120, 0};
	AT(w->o, 1).r = 6;
	AT(w->o, 1).v = v;
	AT(w->o, 1).pp = Vector2Subtract(AT(w->o, w->o.size - 1).p,
					Vector2Scale(v, delta));
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
	reset(&w, 1./60./SUBFRAMES);
	world_add_force(&w, gravity);
	printf("%d\n", w.f.size);
	Vector2 setpos;
	srandom(time(NULL));

	InitWindow(SW, SH, "Physics Simulation");
	SetTargetFPS(60);
	ToggleFullscreen();
	ToggleFullscreen();
	int tick = 0;
	int time_count, time_max;
	while (!WindowShouldClose())
	{
		time_count = 0;
		profile_time[time_count++] = profile();
		double delta = GetFrameTime();
		if (delta > 0.01) delta = 0.01;

		/* Handle Keys */
		if (KEY(R)) reset(&w, delta/SUBFRAMES);
		if (KEY(A)) {
			world_add_object_at(&w, _T(GetMousePosition()), PHY_GRAVITY);
			AT(w.o, w.o.size - 1).r = powf(random()%500 + 500, 1./3);
		}
		if (KEY(BACKSLASH)) hud = !hud;
		if (KEY(SPACE)) paused = !paused;
		BIND(Q, speed);
		if (KEY(S)) {
			world_apply_forces(&w);
			world_step(&w, delta/SUBFRAMES);
			paused = true;
			stepped = true;
		}
		if (IsKeyPressed(KEY_D) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) setpos = GetMousePosition();
		if (IsKeyReleased(KEY_D) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			world_add_object_at(&w, _T(setpos), PHY_GRAVITY);
			/* pp = p - d v */
			Vector2 v = Vector2Subtract(GetMousePosition(), setpos);
			AT(w.o, w.o.size - 1).v = v;
			AT(w.o, w.o.size - 1).pp = Vector2Subtract(AT(w.o, w.o.size - 1).p,
								Vector2Scale(v, delta/SUBFRAMES));
			AT(w.o, w.o.size - 1).r = powf(random()%500 + 500, 1./3);
		}
		if (KEY(F)) {
			float r = 100;
			int mindx = -1;
			for (int i = 0; i < w.o.size; i++) {
				float l = Vector2Distance(GetMousePosition(), AT(w.o, i).p);
				if (l < r) r = l, mindx = i;
			}
			if (mindx >= 0) object_log(mindx, AT(w.o, mindx));
		}
		if (KEY(L))
			for (int i = 0; i < w.o.size; i++)
				if (!(AT(w.o, i).flags&PHY_INVISIBLE))
					object_log(i, AT(w.o, i));
		if (KEY(T)) {
			for (int i = 0; i < time_max; i++)
				printf("%f ", profile_time[i]);
			printf("\n");
		}

		/* Update positions */
		profile_time[time_count++] = profile();
		if (!paused || stepped) {
			for (int i = 0; i < MIN(SAVE_COUNT, w.o.size); i++)
				if (w.o.size)
					position_history[i][pointer] = w.o.list[i].p;
			pointer = (pointer+1)%HIST_SIZE;
		}
		if (!paused) {
			for (int i = 0; i < speed || i < speed; i++)
				for (int j = 0; j < SUBFRAMES; j++) {
					world_apply_forces(&w);
					world_step(&w, delta/SUBFRAMES * (speed >= 0 ? 1 : -1));
				}
		}


		/* Log Energy */
		profile_time[time_count++] = profile();
		if (!paused || stepped) {
			double E = 0;
			/* KE */
			for (int i = 0; i < w.o.size; i++) {
				double e = 0.5 * powf(AT(w.o, i).r, 3) * Vector2LengthSqr((AT(w.o, i).v));
				E += e;
			}
			/* PE */
			for (int i = 0; i < w.o.size; i++) {
				for (int j = 0; j < i; j++) {
					object a = AT(w.o, i), b = AT(w.o, j);
					double e = G*powf(a.r, 3)*powf(b.r, 3)/Vector2Distance(a.p, b.p);
					E -= e;
				}
			}
			fprintf(stderr, "%d %f\n", tick++, E);
		}

		/* Draw */
		profile_time[time_count++] = profile();
		BeginDrawing();
		ClearBackground(BLACK);
		if (hud) {
			DrawFPS(0, 0);
		}
		for (int i = 0; i < MIN(SAVE_COUNT, w.o.size); i++) {
			for (int j = 0; j < HIST_SIZE; j++) {
				int idx = (pointer + HIST_SIZE + j)%HIST_SIZE;
				DrawPixelV(T(position_history[i][idx]),
					(Color) {255, 255, 255, 255.*j/HIST_SIZE});
			}
		}
		for (int i = 0; i < w.o.size; i++)
			object_show(AT(w.o, i));
		EndDrawing();

		profile_time[time_count++] = profile();
		if (time_count > time_max) time_max = time_count;
		stepped = false;
	}
	CloseWindow();
	world_free(&w);

	return 0;
}

void object_log(int idx, object o)
{
	Vector2 p = o.p, v = o.v, a = o.a;
	printf("object %d: (%.3f %.3f) (%.3f %.3f) (%.3f %.3f) %f %08b\n",
		idx, p.x, p.y, v.x, v.y, a.x, a.y, o.r, o.flags);
}

void object_show(object o)
{
	if (!(o.flags&PHY_INVISIBLE)) {
#ifdef DEBUG
		Vector2 v = C(i.v, 0.1);
		DrawLineV(A(AT(o, i).p, v), A(A(AT(o, i).p, v), C(AT(o, i).a, 0.1)), WHITE);
		DrawLineV(AT(o, i).p, Vector2Add(AT(o, i).p, v), RED);
#endif
		DrawCircleV(T(o.p), 3.*o.r/10, (Color) {255. * o.r / 100., 34, 56, 255});
	}
}
