#ifndef __MY__CSTRING_H
#define __MY__CSTRING_H

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

typedef struct
{
	char  *buffer;
	size_t length;
	size_t capacity;
} CString;

void cstr_init( CString *s, char *buf, size_t cap );

void cstr_reset( CString *s );

void cstr_set( CString *s, const char *src );

void cstr_append( CString *s, const char *src );

void cstr_append_char( CString *s, char c );

void cstr_appendf( CString *s, const char *fmt, ... );

#define DECLARE_CSTRING( name, size )                                                                                  \
	char    name##_storage[size];                                                                                      \
	CString name;                                                                                                      \
	cstr_init( &name, name##_storage, size )

#define MAKE_CSTR_LIST( NAME, COUNT, SIZE )                                                                            \
	char    NAME##_storage[COUNT][SIZE];                                                                               \
	CString NAME[COUNT];                                                                                               \
	size_t  NAME##_count = 0;                                                                                          \
	do                                                                                                                 \
	{                                                                                                                  \
		for ( size_t i = 0; i < ( COUNT ); ++i )                                                                       \
			cstr_init( &( NAME )[i], NAME##_storage[i], SIZE );                                                        \
	} while ( 0 )

#define CSTRING_LIST_PUSH( NAME, STR )                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		if ( NAME##_count < sizeof( NAME ) / sizeof( NAME[0] ) )                                                       \
		{                                                                                                              \
			cstr_set( &NAME[NAME##_count++], STR );                                                                    \
		}                                                                                                              \
	} while ( 0 )

#endif // CSTRING_H
