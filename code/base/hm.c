#include "hm.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uint64_t dk_hash_string( const char *str )
{
	uint64_t hash = 14695981039346656037ULL;
	while ( *str )
	{
		hash ^= (unsigned char)*str++;
		hash *= 1099511628211ULL;
	}
	return hash;
}

void hm_init( DK_HashMap *map, size_t initial_capacity )
{
	if ( ( initial_capacity & ( initial_capacity - 1 ) ) != 0 )
	{
		fprintf( stderr, "hm: capacity must be power of 2\n" );
		exit( 1 );
	}

	map->capacity = initial_capacity;
	map->count    = 0;
	map->keys     = (const char **)calloc( initial_capacity, sizeof( char * ) );
	map->values   = (void **)calloc( initial_capacity, sizeof( void * ) );
	map->flags    = (uint8_t *)calloc( initial_capacity, sizeof( uint8_t ) );
}

void hm_free( DK_HashMap *map )
{
	free( map->keys );
	free( map->values );
	free( map->flags );
	map->keys     = NULL;
	map->values   = NULL;
	map->flags    = NULL;
	map->count    = 0;
	map->capacity = 0;
}

void *hm_get( DK_HashMap *map, const char *key )
{
	if ( map->capacity == 0 )
		return NULL;

	uint64_t hash  = dk_hash_string( key );
	size_t   cap   = map->capacity;
	size_t   index = hash & ( cap - 1 );

	for ( size_t i = 0; i < cap; ++i )
	{
		size_t probe = ( index + i ) & ( cap - 1 );
		if ( map->flags[probe] == DK_MAP_EMPTY )
			return NULL;

		if ( map->flags[probe] == DK_MAP_OCCUPIED && strcmp( map->keys[probe], key ) == 0 )
		{
			return map->values[probe];
		}
	}
	return NULL;
}

void hm_put( DK_HashMap *map, const char *key, void *value )
{
	if ( map->count * 2 >= map->capacity )
	{
		fprintf( stderr, "hm: no resizing implemented, capacity full\n" );
		exit( 1 );
	}

	uint64_t hash  = dk_hash_string( key );
	size_t   cap   = map->capacity;
	size_t   index = hash & ( cap - 1 );

	for ( size_t i = 0; i < cap; ++i )
	{
		size_t probe = ( index + i ) & ( cap - 1 );

		if ( map->flags[probe] == DK_MAP_EMPTY || map->flags[probe] == DK_MAP_TOMBSTONE )
		{

			map->keys[probe]   = key;
			map->values[probe] = value;
			map->flags[probe]  = DK_MAP_OCCUPIED;
			map->count++;
			return;
		}

		if ( map->flags[probe] == DK_MAP_OCCUPIED && strcmp( map->keys[probe], key ) == 0 )
		{
			map->values[probe] = value;
			return;
		}
	}
}

void hm_remove( DK_HashMap *map, const char *key )
{
	if ( map->capacity == 0 )
		return;

	uint64_t hash  = dk_hash_string( key );
	size_t   cap   = map->capacity;
	size_t   index = hash & ( cap - 1 );

	for ( size_t i = 0; i < cap; ++i )
	{
		size_t probe = ( index + i ) & ( cap - 1 );
		if ( map->flags[probe] == DK_MAP_EMPTY )
			return;

		if ( map->flags[probe] == DK_MAP_OCCUPIED && strcmp( map->keys[probe], key ) == 0 )
		{
			map->flags[probe]  = DK_MAP_TOMBSTONE;
			map->keys[probe]   = NULL;
			map->values[probe] = NULL;
			map->count--;
			return;
		}
	}
}
