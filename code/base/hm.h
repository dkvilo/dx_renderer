#ifndef DK_MAP_H
#define DK_MAP_H

#include <stdint.h>
#include <stddef.h>

#define DK_MAP_OCCUPIED 1
#define DK_MAP_EMPTY 0
#define DK_MAP_TOMBSTONE 2

typedef struct
{
	const char **keys;
	void       **values;
	uint8_t     *flags;
	size_t       count;
	size_t       capacity;
} DK_HashMap;

void hm_init( DK_HashMap *map, size_t initial_capacity );

void hm_free( DK_HashMap *map );

void *hm_get( DK_HashMap *map, const char *key );

void hm_put( DK_HashMap *map, const char *key, void *value );

void hm_remove( DK_HashMap *map, const char *key );

#endif // DK_MAP_H
