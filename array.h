#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>
#define ARRAY(t)				\
	struct t##_array {			\
		t *list;			\
		unsigned int size, capacity;	\
	}
#define AT(arr, idx) ((arr).list)[idx]
#define array_append(arr, item)						\
	do {								\
		if ((arr).capacity == 0) {				\
			(arr).list = calloc(WORLD_INITIAL_CAP, sizeof (object)); \
			(arr).capacity = WORLD_INITIAL_CAP;		\
		} else if ((arr).capacity < (arr).size) {		\
			perror("Unreachable");				\
		} else if ((arr).capacity == (arr).size) {		\
			(arr).list = reallocarray((arr).list, (arr).capacity *= 2, sizeof (object)); \
		}							\
		if ((arr).list == NULL) {				\
			perror("Allocation");				\
			break;						\
		}							\
		(arr).list[(arr).size++] = (item);			\
	} while(0)
#define array_free(arr) if ((arr).capacity) free((arr).list)


#endif /* ARRAY_H */
