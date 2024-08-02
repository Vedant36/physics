CC=gcc -lm -lraylib
FILES=physics.c physics.h array.h

physics: $(FILES)
	$(CC) physics.c -o physics

debug: $(FILES)
	$(CC) -ggdb physics.c -o physics

test: $(FILES)
	$(CC) -ggdb physics.c -Wall -Werror -Wextra -Wno-sign-compare -o physics
