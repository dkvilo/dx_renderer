#include <stdio.h>

const char *io_read_file_content( const char *path )
{
	FILE *file = fopen( path, "r" );
	if ( !file )
	{
		printf( "Could not open file: %s\n", path );
		return NULL;
	}

	fseek( file, 0, SEEK_END );
	long fileSize = ftell( file );
	fseek( file, 0, SEEK_SET );

	char *buffer = (char *)malloc( fileSize + 1 );
	if ( !buffer )
	{
		printf( "Could not allocate memory for file: %s\n", path );
		fclose( file );
		return NULL;
	}

	size_t bytesRead  = fread( buffer, 1, fileSize, file );
	buffer[bytesRead] = '\0';

	fclose( file );
	return buffer;
}

static void *io_read_bin_content( const char *path, size_t *outSize )
{
	FILE *f = fopen( path, "rb" );
	if ( !f )
		return NULL;

	fseek( f, 0, SEEK_END );
	size_t size = ftell( f );
	fseek( f, 0, SEEK_SET );

	void *buf = malloc( size );
	if ( !buf )
	{
		fclose( f );
		return NULL;
	}

	if ( fread( buf, 1, size, f ) != size )
	{
		free( buf );
		fclose( f );
		return NULL;
	}

	fclose( f );
	if ( outSize )
		*outSize = size;
	return buf;
}
