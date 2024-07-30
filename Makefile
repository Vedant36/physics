CC=gcc -lm -lraylib

physics: physics.c
	$(CC) physics.c -o physics

debug: physics.c
	$(CC) -ggdb physics.c -o physics
