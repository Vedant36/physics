CC=gcc -lm -lraylib
FILES=physics.c physics.h array.h

supafast: $(FILES)
	$(CC) physics.c -O3 -o physics

debug: $(FILES)
	$(CC) -ggdb physics.c -o physics

test: $(FILES)
	$(CC) -ggdb physics.c -Wall -Werror -Wextra -Wno-sign-compare -Wno-sequence-point -o physics
