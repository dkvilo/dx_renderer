#ifndef __MY__STRINGBUILDER_H
#define __MY__STRINGBUILDER_H

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct
{
	char  *buffer;
	size_t capacity;
	size_t length;
} CStringBuilder;

void sb_init( CStringBuilder *sb, char *buf, size_t cap );
void sb_reset( CStringBuilder *sb );
void sb_append( CStringBuilder *sb, const char *str );
void sb_append_char( CStringBuilder *sb, char c );
void sb_appendf( CStringBuilder *sb, const char *fmt, ... );

#define DECLARE_STRING_BUILDER( name, size )                                                                           \
	char           name##_storage[size];                                                                               \
	CStringBuilder name;                                                                                               \
	sb_init( &name, name##_storage, size )

#endif // __MY__STRINGBUILDER_H
