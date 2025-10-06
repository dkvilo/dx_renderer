#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NAME_LEN 64
#define MAX_LINE_LEN 512
#define MAX_FIELDS 32
#define MAX_LAYOUTS 32
#define MAX_DECLARATIONS 32

typedef struct
{
	char dsl_type[MAX_NAME_LEN];
	char name[MAX_NAME_LEN];
	char semantic[MAX_NAME_LEN];
	int  semantic_index;
	int  is_normalized;
} Field;

typedef enum
{
	LAYOUT_TYPE_VERTEX,
	LAYOUT_TYPE_BUFFER
} LayoutType;

typedef struct
{
	char       name[MAX_NAME_LEN];
	LayoutType type;
	Field      fields[MAX_FIELDS];
	int        field_count;
} Layout;

typedef enum
{
	DECL_TYPE_VERTEX,
	DECL_TYPE_PIXEL,
	DECL_TYPE_BUFFER,
	DECL_TYPE_SAMPLER,
	DECL_TYPE_TEXTURE
} DeclarationType;

typedef enum HostExport
{
	NONE        = 0,
	ONLY_CPU    = 1,
	ONLY_GPU    = 2,
	CPU_AND_GPU = 3,
} HostExport;

typedef struct
{
	DeclarationType type;
	char            name[MAX_NAME_LEN];
	char            layout_name[MAX_NAME_LEN];
	char            binding[MAX_NAME_LEN];
	int             is_vertex_stage;
	int             is_pixel_stage;
	int             is_input;
	HostExport      host;
} Declaration;

typedef struct
{
	Layout      layouts[MAX_LAYOUTS];
	int         layout_count;
	Declaration declarations[MAX_DECLARATIONS];
	int         declaration_count;
} ParsedFile;

typedef struct
{
	const char *dsl_type;
	const char *c_base_type;
	int         c_array_size;
	const char *hlsl_type;
	const char *dxgi_format;
} TypeMapping;

static const TypeMapping TYPE_MAPPINGS[] = {
    { "matrix", "float", 16, "matrix", "DXGI_FORMAT_UNKNOWN" },
    { "float4", "float", 4, "float4", "DXGI_FORMAT_R32G32B32A32_FLOAT" },
    { "float3", "float", 3, "float3", "DXGI_FORMAT_R32G32B32_FLOAT" },
    { "float2", "float", 2, "float2", "DXGI_FORMAT_R32G32_FLOAT" },
    { "float", "float", 1, "float", "DXGI_FORMAT_R32_FLOAT" },
    { "u8x4", "uint8_t", 4, "float4", "DXGI_FORMAT_R8G8B8A8_UNORM" },
    { "uint", "unsigned int", 1, "uint", "DXGI_FORMAT_R32_UINT" },
    { "int4", "int", 4, "int4", "DXGI_FORMAT_R32G32B32A32_SINT" },
    { "int3", "int", 3, "int3", "DXGI_FORMAT_R32G32B32_SINT" },
    { "int2", "int", 2, "int2", "DXGI_FORMAT_R32G32_SINT" },
    { "int", "int", 1, "int", "DXGI_FORMAT_R32_SINT" },
    { "uint4", "unsigned int", 4, "uint4", "DXGI_FORMAT_R32G32B32A32_UINT" },
    { "uint3", "unsigned int", 3, "uint3", "DXGI_FORMAT_R32G32B32_UINT" },
    { "uint2", "unsigned int", 2, "uint2", "DXGI_FORMAT_R32G32_UINT" },
    { "i16x4", "int16_t", 4, "int4", "DXGI_FORMAT_R16G16B16A16_SINT" },
    { "i16x3", "int16_t", 3, "int3", "DXGI_FORMAT_R16G16B16A16_SINT" },
    { "i16x2", "int16_t", 2, "int2", "DXGI_FORMAT_R16G16_SINT" },
    { "i16", "int16_t", 1, "int", "DXGI_FORMAT_R16_SINT" },
    { "u16x4", "uint16_t", 4, "uint4", "DXGI_FORMAT_R16G16B16A16_UINT" },
    { "u16x3", "uint16_t", 3, "uint3", "DXGI_FORMAT_R16G16B16A16_UINT" },
    { "u16x2", "uint16_t", 2, "uint2", "DXGI_FORMAT_R16G16_UINT" },
    { "u16", "uint16_t", 1, "uint", "DXGI_FORMAT_R16_UINT" },
    { "u16x4_norm", "uint16_t", 4, "float4", "DXGI_FORMAT_R16G16B16A16_UNORM" },
    { "u16x2_norm", "uint16_t", 2, "float2", "DXGI_FORMAT_R16G16_UNORM" },
    { "u16_norm", "uint16_t", 1, "float", "DXGI_FORMAT_R16_UNORM" },
    { "i16x4_norm", "int16_t", 4, "float4", "DXGI_FORMAT_R16G16B16A16_SNORM" },
    { "i16x2_norm", "int16_t", 2, "float2", "DXGI_FORMAT_R16G16_SNORM" },
    { "i16_norm", "int16_t", 1, "float", "DXGI_FORMAT_R16_SNORM" },
    { "i8x4", "int8_t", 4, "int4", "DXGI_FORMAT_R8G8B8A8_SINT" },
    { "i8x2", "int8_t", 2, "int2", "DXGI_FORMAT_R8G8_SINT" },
    { "i8", "int8_t", 1, "int", "DXGI_FORMAT_R8_SINT" },
    { "u8x4", "uint8_t", 4, "uint4", "DXGI_FORMAT_R8G8B8A8_UINT" },
    { "u8x2", "uint8_t", 2, "uint2", "DXGI_FORMAT_R8G8_UINT" },
    { "u8", "uint8_t", 1, "uint", "DXGI_FORMAT_R8_UINT" },
    { "i8x4_norm", "int8_t", 4, "float4", "DXGI_FORMAT_R8G8B8A8_SNORM" },
    { "i8x2_norm", "int8_t", 2, "float2", "DXGI_FORMAT_R8G8_SNORM" },
    { "i8_norm", "int8_t", 1, "float", "DXGI_FORMAT_R8_SNORM" },
    { "half4", "uint16_t", 4, "float4", "DXGI_FORMAT_R16G16B16A16_FLOAT" },
    { "half2", "uint16_t", 2, "float2", "DXGI_FORMAT_R16G16_FLOAT" },
    { "half", "uint16_t", 1, "float", "DXGI_FORMAT_R16_FLOAT" },
    { "u10u10u10u2", "uint32_t", 1, "float4", "DXGI_FORMAT_R10G10B10A2_UNORM" },
    { "u10u10u10u2_uint", "uint32_t", 1, "uint4", "DXGI_FORMAT_R10G10B10A2_UINT" },
    { "u11u11u10", "uint32_t", 1, "float3", "DXGI_FORMAT_R11G11B10_FLOAT" },
    { "bgra8", "uint8_t", 4, "float4", "DXGI_FORMAT_B8G8R8A8_UNORM" },
    { "bgrx8", "uint8_t", 4, "float3", "DXGI_FORMAT_B8G8R8X8_UNORM" },
    { "rgba8_srgb", "uint8_t", 4, "float4", "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB" },
    { "bgra8_srgb", "uint8_t", 4, "float4", "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB" },
    { "r9g9b9e5", "uint32_t", 1, "float3", "DXGI_FORMAT_R9G9B9E5_SHAREDEXP" },
    { "double4", "double", 4, "double4", "DXGI_FORMAT_UNKNOWN" },
    { "double2", "double", 2, "double2", "DXGI_FORMAT_UNKNOWN" },
    { "double", "double", 1, "double", "DXGI_FORMAT_UNKNOWN" },
    { "color", "uint8_t", 4, "float4", "DXGI_FORMAT_R8G8B8A8_UNORM" },
    { "color_bgra", "uint8_t", 4, "float4", "DXGI_FORMAT_B8G8R8A8_UNORM" },
    { "normal", "int8_t", 4, "float4", "DXGI_FORMAT_R8G8B8A8_SNORM" },
    { "normal16", "int16_t", 4, "float4", "DXGI_FORMAT_R16G16B16A16_SNORM" },
    { "index16", "uint16_t", 1, "uint", "DXGI_FORMAT_R16_UINT" },
    { "index32", "uint32_t", 1, "uint", "DXGI_FORMAT_R32_UINT" },
    { "bone_weights", "uint8_t", 4, "float4", "DXGI_FORMAT_R8G8B8A8_UNORM" },
    { "bone_indices", "uint8_t", 4, "uint4", "DXGI_FORMAT_R8G8B8A8_UINT" },
};
static const int NUM_TYPE_MAPPINGS = sizeof( TYPE_MAPPINGS ) / sizeof( TYPE_MAPPINGS[0] );

static TypeMapping get_type_mapping( const char *dsl_type )
{
	for ( int i = 0; i < NUM_TYPE_MAPPINGS; ++i )
	{
		if ( strcmp( dsl_type, TYPE_MAPPINGS[i].dsl_type ) == 0 )
		{
			return TYPE_MAPPINGS[i];
		}
	}
	fprintf( stderr, "Warning: Unknown DSL type '%s'. Falling back to float4.\n", dsl_type );
	static const TypeMapping fallback = { "unknown", "float", 4, "float4", "DXGI_FORMAT_R32G32B32A32_FLOAT" };
	return fallback;
}

static char *trim_whitespace( char *s );

static int parse_field_line( char *line, Field *field );

static int parse_layout( FILE *f, Layout *layout );

static int parse_declaration_attributes( char *attr_str, Declaration *decl );

static int parse_file( const char *path, ParsedFile *parsed );

static Layout *find_layout( ParsedFile *parsed, const char *name );

static void generate_header_file( FILE *hf, ParsedFile *parsed, const char *input_path, const char *header_guard );

static void generate_hlsl_file( FILE *hfsl, ParsedFile *parsed, const char *input_path );

static void write_output_files( const char *output_basename, const char *input_path, ParsedFile *parsed );

static char *trim_whitespace( char *s )
{
	while ( *s && isspace( (unsigned char)*s ) )
		s++;
	if ( *s == 0 )
		return s;

	char *end = s + strlen( s ) - 1;
	while ( end > s && isspace( (unsigned char)*end ) )
		*end-- = '\0';
	return s;
}

static int parse_field_line( char *line, Field *field )
{
	char *semi = strchr( line, ';' );
	if ( semi )
		*semi = '\0';
	char *p = trim_whitespace( line );
	if ( *p == '\0' )
		return 0;

	char type_tok[MAX_NAME_LEN] = { 0 };
	char name_tok[MAX_NAME_LEN] = { 0 };
	int  n                      = 0;
	if ( sscanf( p, "%63s %63s %n", type_tok, name_tok, &n ) < 2 )
		return 0;

	p += n;
	strncpy( field->dsl_type, type_tok, sizeof( field->dsl_type ) - 1 );
	strncpy( field->name, name_tok, sizeof( field->name ) - 1 );

	field->semantic[0]    = '\0';
	field->semantic_index = 0;
	field->is_normalized  = 0;

	char *colon = strchr( p, ':' );
	if ( colon )
	{
		p                         = trim_whitespace( colon + 1 );
		char semtok[MAX_NAME_LEN] = { 0 };
		int  consumed             = 0;
		if ( sscanf( p, "%63s %n", semtok, &consumed ) >= 1 )
		{
			char *p_num = semtok + strlen( semtok );
			while ( p_num > semtok && isdigit( (unsigned char)p_num[-1] ) )
			{
				p_num--;
			}

			if ( *p_num && isdigit( (unsigned char)*p_num ) )
			{
				field->semantic_index = atoi( p_num );
				*p_num                = '\0';
			}
			strncpy( field->semantic, semtok, sizeof( field->semantic ) - 1 );

			p += consumed;
			field->is_normalized = ( strstr( p, "normalized" ) || strstr( p, "norm" ) ) ? 1 : 0;
		}
	}
	return 1;
}

static int parse_layout( FILE *f, Layout *layout )
{
	char line[MAX_LINE_LEN];
	layout->field_count = 0;

	while ( fgets( line, sizeof( line ), f ) )
	{
		char *t = trim_whitespace( line );
		if ( *t == '\0' || strncmp( t, "//", 2 ) == 0 )
			continue;

		if ( *t == '}' )
			break;

		if ( layout->field_count >= MAX_FIELDS )
		{
			fprintf( stderr, "Error: Too many fields in layout '%s'. Max is %d.\n", layout->name, MAX_FIELDS );
			return 0;
		}

		if ( parse_field_line( t, &layout->fields[layout->field_count] ) )
		{
			layout->field_count++;
		}
		else
		{
			fprintf( stderr, "Warning: Failed to parse field line: %s\n", t );
		}
	}
	return layout->field_count > 0;
}

static int parse_declaration_attributes( char *attr_str, Declaration *decl )
{
	char local_attr_str[MAX_LINE_LEN];
	strncpy( local_attr_str, attr_str, sizeof( local_attr_str ) - 1 );
	local_attr_str[sizeof( local_attr_str ) - 1] = '\0';

	decl->is_vertex_stage = 0;
	decl->is_pixel_stage  = 0;
	decl->is_input        = 0;
	decl->binding[0]      = '\0';

	char *token = strtok( local_attr_str, " \t;" );
	while ( token != NULL )
	{
		if ( strcmp( token, "@vertex" ) == 0 )
			decl->is_vertex_stage = 1;
		else if ( strcmp( token, "@pixel" ) == 0 )
			decl->is_pixel_stage = 1;
		else if ( strcmp( token, "@input" ) == 0 )
			decl->is_input = 1;
		else if ( strcmp( token, "@gpu" ) == 0 )
		{
			decl->host = ONLY_GPU;
		}
		else if ( strcmp( token, "@cpu" ) == 0 )
		{
			decl->host = ONLY_CPU;
		}
		else if ( strcmp( token, "@cpu_gpu" ) == 0 )
		{
			decl->host = CPU_AND_GPU;
		}

		else if ( token[0] == '@' && token[1] == 'b' && isdigit( (unsigned char)token[2] ) )
		{
			strncpy( decl->binding, token, sizeof( decl->binding ) - 1 );
		}
		token = strtok( NULL, " \t;" );
	}
	return 1;
}

static int parse_file( const char *path, ParsedFile *parsed )
{
	FILE *f = fopen( path, "r" );
	if ( !f )
	{
		fprintf( stderr, "Error: Failed to open input file: %s\n", path );
		return 0;
	}

	parsed->layout_count      = 0;
	parsed->declaration_count = 0;
	char line[MAX_LINE_LEN];
	int  line_num = 0;

	while ( fgets( line, sizeof( line ), f ) )
	{
		line_num++;
		char *t = trim_whitespace( line );
		if ( *t == '\0' || strncmp( t, "//", 2 ) == 0 )
			continue;

#define PARSE_DECL( keyword, type_enum )                                                                               \
	if ( strncmp( t, keyword, strlen( keyword ) ) == 0 && isspace( (unsigned char)t[strlen( keyword )] ) )             \
	{                                                                                                                  \
		if ( parsed->declaration_count >= MAX_DECLARATIONS )                                                           \
		{                                                                                                              \
			fprintf( stderr, "Error: Too many declarations. Max is %d.\n", MAX_DECLARATIONS );                         \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			Declaration *decl            = &parsed->declarations[parsed->declaration_count];                           \
			decl->type                   = type_enum;                                                                  \
			char  name_tok[MAX_NAME_LEN] = { 0 };                                                                      \
			char *rest                   = t + strlen( keyword );                                                      \
			int   n                      = 0;                                                                          \
			if ( sscanf( rest, "%63s %n", name_tok, &n ) == 1 )                                                        \
			{                                                                                                          \
				if ( type_enum == DECL_TYPE_TEXTURE || type_enum == DECL_TYPE_SAMPLER )                                \
				{                                                                                                      \
					strncpy( decl->name, name_tok, sizeof( decl->name ) - 1 );                                         \
					decl->layout_name[0] = '\0';                                                                       \
				}                                                                                                      \
				else                                                                                                   \
				{                                                                                                      \
					strncpy( decl->layout_name, name_tok, sizeof( decl->layout_name ) - 1 );                           \
					decl->name[0] = '\0';                                                                              \
				}                                                                                                      \
				parse_declaration_attributes( rest + n, decl );                                                        \
				parsed->declaration_count++;                                                                           \
			}                                                                                                          \
			else                                                                                                       \
			{                                                                                                          \
				fprintf( stderr, "Error: Malformed declaration on line %d: %s\n", line_num, t );                       \
			}                                                                                                          \
		}                                                                                                              \
		continue;                                                                                                      \
	}

		if ( strncmp( t, "layout", 6 ) == 0 && isspace( (unsigned char)t[6] ) )
		{
			if ( parsed->layout_count >= MAX_LAYOUTS )
			{
				fprintf( stderr, "Error: Too many layouts. Max is %d.\n", MAX_LAYOUTS );
			}
			else
			{
				Layout *layout             = &parsed->layouts[parsed->layout_count];
				char    name[MAX_NAME_LEN] = { 0 };
				if ( sscanf( t + 6, "%63s", name ) == 1 )
				{
					strncpy( layout->name, name, sizeof( layout->name ) - 1 );
					layout->type = LAYOUT_TYPE_VERTEX;

					char *brace = strchr( t, '{' );
					if ( !brace )
					{
						while ( fgets( line, sizeof( line ), f ) )
						{
							line_num++;
							if ( strchr( line, '{' ) )
								break;
						}
					}

					if ( parse_layout( f, layout ) )
					{
						parsed->layout_count++;
					}
					else
					{
						fprintf( stderr, "Error: Failed to parse layout '%s' on line %d.\n", name, line_num );
					}
				}
			}
			continue;
		}

		PARSE_DECL( "vertex", DECL_TYPE_VERTEX );
		PARSE_DECL( "pixel", DECL_TYPE_PIXEL );
		PARSE_DECL( "buffer", DECL_TYPE_BUFFER );
		PARSE_DECL( "sampler", DECL_TYPE_SAMPLER );
		PARSE_DECL( "texture", DECL_TYPE_TEXTURE );
	}

	fclose( f );
	return 1;
}

static Layout *find_layout( ParsedFile *parsed, const char *name )
{
	for ( int i = 0; i < parsed->layout_count; i++ )
	{
		if ( strcmp( parsed->layouts[i].name, name ) == 0 )
		{
			return &parsed->layouts[i];
		}
	}
	return NULL;
}

static void generate_header_file( FILE *hf, ParsedFile *parsed, const char *input_path, const char *header_guard )
{
	fprintf( hf,
	         "/**\n * @file\n * @brief Auto-generated file from %s.\n * Do not edit manually.\n */\n\n",
	         input_path );
	fprintf( hf, "#ifndef %s\n#define %s\n\n", header_guard, header_guard );
	fprintf( hf, "#include <stdint.h>\n#include <d3d11.h>\n#include <stddef.h>\n\n" );

	int processed_layouts[MAX_LAYOUTS] = { 0 };

	for ( int i = 0; i < parsed->declaration_count; i++ )
	{
		Declaration *decl   = &parsed->declarations[i];
		Layout      *layout = find_layout( parsed, decl->layout_name );
		if ( !layout || decl->host == ONLY_GPU )
			continue;

		int layout_idx = layout - parsed->layouts;
		if ( processed_layouts[layout_idx] )
			continue;

		fprintf( hf, "typedef struct %s {\n", layout->name );
		for ( int f_idx = 0; f_idx < layout->field_count; f_idx++ )
		{
			Field      *field   = &layout->fields[f_idx];
			TypeMapping mapping = get_type_mapping( field->dsl_type );
			if ( mapping.c_array_size > 1 )
			{
				fprintf( hf, "    %s %s[%d];\n", mapping.c_base_type, field->name, mapping.c_array_size );
			}
			else
			{
				fprintf( hf, "    %s %s;\n", mapping.c_base_type, field->name );
			}
		}
		fprintf( hf, "} %s;\n\n", layout->name );

		if ( decl->type == DECL_TYPE_VERTEX && decl->is_input )
		{
			fprintf( hf, "static const D3D11_INPUT_ELEMENT_DESC %s_desc[] = {\n", layout->name );
			for ( int f_idx = 0; f_idx < layout->field_count; f_idx++ )
			{
				Field *field = &layout->fields[f_idx];
				if ( strlen( field->semantic ) == 0 )
					continue;

				TypeMapping mapping = get_type_mapping( field->dsl_type );
				fprintf( hf,
				         "    { \"%s\", %d, %s, 0, offsetof(%s, %s), D3D11_INPUT_PER_VERTEX_DATA, 0 },\n",
				         field->semantic,
				         field->semantic_index,
				         mapping.dxgi_format,
				         layout->name,
				         field->name );
			}
			fprintf( hf, "};\n" );
			fprintf( hf,
			         "static const unsigned int %s_desc_count = sizeof(%s_desc) / sizeof(%s_desc[0]);\n\n",
			         layout->name,
			         layout->name,
			         layout->name );
		}
		processed_layouts[layout_idx] = 1;
	}

	fprintf( hf, "#endif // %s\n", header_guard );
}

static void generate_hlsl_file( FILE *hfsl, ParsedFile *parsed, const char *input_path )
{
	fprintf( hfsl,
	         "/**\n * @file\n * @brief Auto-generated file from %s.\n * Do not edit manually.\n */\n\n",
	         input_path );

	for ( int i = 0; i < parsed->declaration_count; i++ )
	{
		Declaration *decl   = &parsed->declarations[i];
		Layout      *layout = find_layout( parsed, decl->layout_name );

		if ( decl->type == DECL_TYPE_TEXTURE || decl->type == DECL_TYPE_SAMPLER )
		{
			if ( decl->type == DECL_TYPE_SAMPLER )
			{
				int reg_num = ( strlen( decl->binding ) > 2 ) ? atoi( &decl->binding[2] ) : 0;
				fprintf( hfsl, "SamplerState %s : register(s%d);\n", decl->name, reg_num );
			}
			else if ( decl->type == DECL_TYPE_TEXTURE )
			{
				int reg_num = ( strlen( decl->binding ) > 2 ) ? atoi( &decl->binding[2] ) : 0;
				fprintf( hfsl, "Texture2D %s : register(t%d);\n", decl->name, reg_num );
			}
			continue;
		}

		if ( !layout || decl->host == ONLY_CPU )
		{
			fprintf( stderr,
			         "Warning: Layout '%s' not found for declaration, skipping HLSL generation for it.\n",
			         decl->layout_name );
			continue;
		}

		if ( decl->type == DECL_TYPE_VERTEX && decl->is_input )
		{
			fprintf( hfsl, "struct VS_INPUT {\n" );
		}
		else if ( decl->type == DECL_TYPE_PIXEL && decl->is_input )
		{
			fprintf( hfsl, "struct PS_INPUT {\n" );
		}
		else if ( decl->type == DECL_TYPE_BUFFER )
		{
			int reg_num = ( strlen( decl->binding ) > 2 ) ? atoi( &decl->binding[2] ) : 0;
			fprintf( hfsl, "cbuffer %s : register(b%d) {\n", layout->name, reg_num );
		}
		else
		{
			continue;
		}

		for ( int f_idx = 0; f_idx < layout->field_count; f_idx++ )
		{
			Field      *field   = &layout->fields[f_idx];
			TypeMapping mapping = get_type_mapping( field->dsl_type );

			if ( decl->type == DECL_TYPE_BUFFER )
			{
				fprintf( hfsl, "    %s %s;\n", mapping.hlsl_type, field->name );
			}
			else
			{
				fprintf( hfsl, "    %s %s : %s;\n", mapping.hlsl_type, field->name, field->semantic );
			}
		}
		fprintf( hfsl, "};\n\n" );
	}
}

static void write_output_files( const char *output_basename, const char *input_path, ParsedFile *parsed )
{
	char h_path[MAX_LINE_LEN], hlsl_path[MAX_LINE_LEN], header_guard[MAX_NAME_LEN];

	snprintf( h_path, sizeof( h_path ), "%s.h", output_basename );
	snprintf( hlsl_path, sizeof( hlsl_path ), "%s.hlsl", output_basename );

	const char *base_ptr = strrchr( output_basename, '/' );
	base_ptr             = base_ptr ? base_ptr + 1 : strrchr( output_basename, '\\' );
	base_ptr             = base_ptr ? base_ptr + 1 : output_basename;
	int i                = 0;
	for ( const char *p = base_ptr; *p && i < sizeof( header_guard ) - 3; ++p, ++i )
	{
		header_guard[i] = isalnum( (unsigned char)*p ) ? toupper( (unsigned char)*p ) : '_';
	}
	snprintf( header_guard + i, sizeof( header_guard ) - i, "_H_" );

	FILE *hf   = fopen( h_path, "w" );
	FILE *hfsl = fopen( hlsl_path, "w" );

	if ( !hf || !hfsl )
	{
		fprintf( stderr, "Error: Failed to open one or more output files for writing.\n" );
		if ( hf )
			fclose( hf );
		if ( hfsl )
			fclose( hfsl );
		return;
	}

	generate_header_file( hf, parsed, input_path, header_guard );
	generate_hlsl_file( hfsl, parsed, input_path );

	fclose( hf );
	fclose( hfsl );

	printf( "Successfully generated: %s and %s\n", h_path, hlsl_path );
}

int main( int argc, char **argv )
{
	if ( argc < 3 )
	{
		fprintf( stderr, "Usage: %s <input.vtx> <output_basename>\n", argv[0] );
		fprintf( stderr, "  Example: %s my_shader.vtx my_shader_generated\n", argv[0] );
		fprintf( stderr, "  This will generate 'my_shader_generated.h' and 'my_shader_generated.hlsl'\n" );
		return EXIT_FAILURE;
	}

	const char *input_file      = argv[1];
	const char *output_basename = argv[2];

	ParsedFile parsed;
	if ( !parse_file( input_file, &parsed ) )
	{
		fprintf( stderr, "Error: Failed to parse file '%s'. Aborting.\n", input_file );
		return EXIT_FAILURE;
	}

	printf( "Parsed %s: %d layouts and %d declarations.\n", input_file, parsed.layout_count, parsed.declaration_count );
	write_output_files( output_basename, input_file, &parsed );

	return EXIT_SUCCESS;
}
