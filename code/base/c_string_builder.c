#include "c_string_builder.h"

void sb_init( CStringBuilder *sb, char *buf, size_t cap )
{
	sb->buffer   = buf;
	sb->capacity = cap;
	sb->length   = 0;
	if ( cap > 0 )
		sb->buffer[0] = '\0';
}

void sb_reset( CStringBuilder *sb )
{
	sb->length = 0;
	if ( sb->capacity > 0 )
		sb->buffer[0] = '\0';
}

void sb_append( CStringBuilder *sb, const char *str )
{
	size_t max_write = sb->capacity - sb->length - 1;
	size_t len       = 0;
	while ( str[len] && len < max_write )
	{
		sb->buffer[sb->length++] = str[len++];
	}
	sb->buffer[sb->length] = '\0';
}

void sb_append_char( CStringBuilder *sb, char c )
{
	if ( sb->length + 1 < sb->capacity )
	{
		sb->buffer[sb->length++] = c;
		sb->buffer[sb->length]   = '\0';
	}
}

void sb_appendf( CStringBuilder *sb, const char *fmt, ... )
{
	size_t space = sb->capacity - sb->length;
	if ( space <= 1 )
		return;

	va_list args;
	va_start( args, fmt );
	int written = vsnprintf( sb->buffer + sb->length, space, fmt, args );
	va_end( args );

	if ( written > 0 )
	{
		size_t w = (size_t)written;
		if ( w >= space )
			w = space - 1;
		sb->length += w;
	}
}
