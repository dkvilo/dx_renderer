#include "c_string.h"

void cstr_init( CString *s, char *buf, size_t cap )
{
	s->buffer   = buf;
	s->length   = 0;
	s->capacity = cap;
	if ( cap > 0 )
		s->buffer[0] = '\0';
}

void cstr_reset( CString *s )
{
	s->length = 0;
	if ( s->capacity > 0 )
		s->buffer[0] = '\0';
}

void cstr_set( CString *s, const char *src )
{
	size_t i = 0;
	while ( src[i] && i + 1 < s->capacity )
	{
		s->buffer[i] = src[i];
		i++;
	}
	s->length    = i;
	s->buffer[i] = '\0';
}

void cstr_append( CString *s, const char *src )
{
	size_t i = 0;
	while ( src[i] && s->length + 1 < s->capacity )
	{
		s->buffer[s->length++] = src[i++];
	}
	s->buffer[s->length] = '\0';
}

void cstr_append_char( CString *s, char c )
{
	if ( s->length + 1 < s->capacity )
	{
		s->buffer[s->length++] = c;
		s->buffer[s->length]   = '\0';
	}
}

inline void cstr_appendf( CString *s, const char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	size_t space = s->capacity - s->length;
	if ( space > 1 )
	{
		int written = vsnprintf( s->buffer + s->length, space, fmt, args );
		if ( written > 0 )
		{
			size_t w = (size_t)written;
			if ( w >= space )
				w = space - 1;
			s->length += w;
		}
	}
	va_end( args );
}
