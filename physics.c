/* TODO:
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

#define SW 500
#define SH 500
#define PHYSICS_IMPLEMENTATION
#include "physics.h"

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
Vector2 origin = {SW/2, SH/2};
float zoom = 1;
const float CR = (SW > SH ? SH : SW) / 2;
#define G  10			/* Gravity coefficient */
#define SUBFRAMES 10.
#define KEY(k) (IsKeyPressed(KEY_##k) || IsKeyPressedRepeat(KEY_##k))
#define BIND(key, param) if (KEY(key)) param += 1 - 2 * IsKeyDown(KEY_LEFT_SHIFT)
frame g;
Vector2 _t;
/* position to screen and inverse */
#define T(v) (_t = v, (Vector2) {(_t.x - g.p.x)*zoom + origin.x, (-_t.y + g.p.y)*zoom + origin.y})
#define _T(v) (_t = v, (Vector2) {(_t.x - origin.x)/zoom + g.p.x, -(_t.y - origin.y)/zoom + g.p.y})
#define LOGV(v) (_t = v, printf("(%f %f)\n", _t.x, _t.y))
#define MASS(o) (powf((o).r, 3))

enum flags {
	PHY_INVISIBLE = 0x1,
	PHY_GRAVITY = 0x2,
	PHY_MASSLESS = 0x4,
	PHY_UNIVERSAL_GRAVITY = 0x8,
};

#define SAVE_COUNT 256
#define HIST_SIZE 4096
Vector2 position_history[SAVE_COUNT][HIST_SIZE] = {0};
int pointer = 0;
float speed = 0;
bool paused, hud, stepped = false;
void reset(world *w, double delta)
{
	g.p = Vector2Zero();
	w->o.size = 0;
	hud = true;
	paused = true;
	pointer = 0;
	speed = 1;
	zoom = 1;
	world_add_object(w,       Vector2Zero(), (Vector2) {-0.0432, 0}, 100, PHY_GRAVITY, delta);
	world_add_object(w, (Vector2) {0, -150}, (Vector2) {200, 0},   6, PHY_GRAVITY, delta);
	for (int i = 0; i < SAVE_COUNT; i++) {
		for (int j = 0; j < HIST_SIZE; j++) {
			position_history[i][j] = Vector2Zero();
		}
	}
}

void object_log(int idx, object o);
void object_show(object o);
Vector2 gravity(world *w, unsigned int i);
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
	Rectangle screen = {0, 0, SW, SH};
	SetTargetFPS(60);
	ToggleFullscreen();
	ToggleFullscreen();
	int tick = 0;
	int time_count, time_max = 0;
	while (!WindowShouldClose())
	{
		time_count = 0;
		profile_time[time_count++] = profile();
		double delta = GetFrameTime();
		if (delta > 1./30) delta = 1./30;

		/* Handle Keys */
		if (KEY(R)) reset(&w, delta/SUBFRAMES);
		if (KEY(A))
			world_add_object(&w, _T(GetMousePosition()), Vector2Zero(), random()%16+4, PHY_GRAVITY, delta);
		if (KEY(BACKSLASH)) hud = !hud;
		if (KEY(SPACE)) paused = !paused;
		BIND(Q, speed);
		if (KEY(Z)) {
			if (IsKeyDown(KEY_LEFT_SHIFT))
				zoom /= 2;
			else
				zoom *= 2;
		}
		if (KEY(S)) {
			world_apply_forces(&w);
			world_step(&w, delta/SUBFRAMES);
			paused = true;
			stepped = true;
		}
		if (IsKeyPressed(KEY_D) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			setpos = GetMousePosition();
		if (IsKeyReleased(KEY_D) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
			world_add_object(&w, _T(setpos),S(_T(GetMousePosition()), _T(setpos)),
					random()%16+4, PHY_GRAVITY, delta/SUBFRAMES);
		if (KEY(F)) {
			int mindx;
			array_max(w.o, -Vector2Distance(GetMousePosition(), T(o.p)), -100, mindx);
			/* float r = 100; */
			/* int mindx = -1; */
			/* for (int i = 0; i < w.o.size; i++) { */
			/* 	float l = Vector2Distance(GetMousePosition(), AT(w.o, i).p); */
			/* 	if (l < r) r = l, mindx = i; */
			/* } */
			if (mindx >= 0) object_log(mindx, AT(w.o, mindx));

		}
		if (KEY(L))
			for (int i = 0; i < w.o.size; i++)
				if (!(AT(w.o, i).flags&PHY_INVISIBLE) && CheckCollisionPointRec(T(AT(w.o, i).p), screen))
					object_log(i, AT(w.o, i));
		if (KEY(T)) {
			printf("Timeinfo: ");
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
				double e = 0.5 * MASS(AT(w.o, i)) * Vector2LengthSqr((AT(w.o, i).v));
				E += e;
			}
			/* PE */
			for (int i = 0; i < w.o.size; i++) {
				for (int j = 0; j < i; j++) {
					object a = AT(w.o, i), b = AT(w.o, j);
					double e = G*MASS(a)*MASS(b)/Vector2Distance(a.p, b.p);
					E -= e;
				}
			}
			fprintf(stderr, "%d %f\n", tick++, E);
		}

		/* Log Momentum */
		profile_time[time_count++] = profile();
		if (!paused || stepped) {
			Vector2 P = {0};
			for (int i = 0; i < w.o.size; i++) {
				Vector2 p = C(AT(w.o, i).v, MASS(AT(w.o, i)));
			        P = A(P, p);
			}
			/* fprintf(stderr, "%d %f\n", tick++, L(P)); */
		}

		/* Frame: com of all objects on screen */
		Vector2 com = Vector2Zero();
		float total_mass = 0;
		for (int i = 0; i < w.o.size; i++) {
			object o = AT(w.o, i);
			if (!CheckCollisionPointRec(T(o.p), screen)) continue;
			com = A(com, C(o.p, MASS(o)));
			total_mass += MASS(o);
		}
		g.p = total_mass > 0.1 ? C(com, 1./total_mass) : Vector2Zero();

		/* Draw */
		profile_time[time_count++] = profile();
		BeginDrawing();
		ClearBackground(BLACK);
		if (hud) {
			DrawFPS(0, 0);
			char buf[1024];
			snprintf(buf, 1024, "Speed: %3.0f", speed);
			DrawText(buf, 0, 16, 16, WHITE);
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
		DrawCircleV(T(o.p), 3.*o.r*zoom/10, (Color) {255. * o.r / 100., 34, 56, 255});
	}
}

/* The interface to choosing which objects to apply the force to is
 * done by flags */
Vector2 gravity(world *w, unsigned int i)
{
	object *p = &AT(w->o, i);
	if (p->flags&PHY_UNIVERSAL_GRAVITY) {
		return (Vector2) {0, G};
	}
	if (p->flags&PHY_GRAVITY) {
		for (int j = 0; j < w->o.size; j++) {
			object *q = &AT(w->o, j);
			if (j == i || q->flags&PHY_MASSLESS) continue;
			Vector2 r = Vector2Subtract(q->p, p->p);
			float l = Vector2Length(r);
			return Vector2Scale(r, MASS(*q) * G/(l*l*l + 1));
		}
	}
	return Vector2Zero();
}

