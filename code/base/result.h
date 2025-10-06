/**
 * @file RESULT.H
 * @brief A generic Result type implementation in C99.
 *
 * This header defines a macro to create a Result type for any data type,
 * along with utility functions and macros to work with the Result type.
 * The Result type can represent either a successful value or an error with
 * an error code and message.
 *
 * Authored by: David Kviloria <david@skystargames.com>
 *
 * Example:
 *
 * DEFINE_RESULT_TYPE(IntResult, int);
 *
 * IntResult res = MAKE_OK(IntResult, 42);
 * if (IntResult_Is_Err(&res)) {
 *   printf("Error (%d): %s\n", res.err.code, res.err.message);
 * } else {
 *   printf("Value: %d\n", res.ok.value);
 * }
 */
#ifndef RESULT_H
#define RESULT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* Maximum length for error messages */
#define MAX_ERROR_MESSAGE_LENGTH 256

/* Macro to define a Result type for a given data type T */
#define DEFINE_RESULT_TYPE( Name, T )                                                                                  \
	typedef struct Name##_Error                                                                                        \
	{                                                                                                                  \
		int32_t code;                                                                                                  \
		char    message[MAX_ERROR_MESSAGE_LENGTH];                                                                     \
	} Name##_Error;                                                                                                    \
                                                                                                                       \
	typedef struct Name##_OK                                                                                           \
	{                                                                                                                  \
		T value;                                                                                                       \
	} Name##_OK;                                                                                                       \
                                                                                                                       \
	typedef enum Name##_Tag{ Name##_TAG_OK, Name##_TAG_ERR } Name##_Tag;                                               \
                                                                                                                       \
	typedef struct Name                                                                                                \
	{                                                                                                                  \
		Name##_Tag tag;                                                                                                \
		union                                                                                                          \
		{                                                                                                              \
			Name##_OK    ok;                                                                                           \
			Name##_Error err;                                                                                          \
		};                                                                                                             \
	} Name;                                                                                                            \
                                                                                                                       \
	static inline bool Name##_Is_Ok( const Name *res )                                                                 \
	{                                                                                                                  \
		return res->tag == Name##_TAG_OK;                                                                              \
	}                                                                                                                  \
	static inline bool Name##_Is_Err( const Name *res )                                                                \
	{                                                                                                                  \
		return res->tag == Name##_TAG_ERR;                                                                             \
	}                                                                                                                  \
	static inline Name Name##_Ok( T value )                                                                            \
	{                                                                                                                  \
		return (Name){ .tag = Name##_TAG_OK, .ok = { value } };                                                        \
	}                                                                                                                  \
	static inline Name Name##_Err( int32_t code, const char *msg )                                                     \
	{                                                                                                                  \
		Name err = { .tag = Name##_TAG_ERR, .err = { code, "" } };                                                     \
		snprintf( err.err.message, sizeof( err.err.message ), "%s", msg );                                             \
		return err;                                                                                                    \
	}                                                                                                                  \
	static inline Name Name##_Err_Debug( int32_t code, const char *msg, const char *file, int line )                   \
	{                                                                                                                  \
		Name err = { .tag = Name##_TAG_ERR };                                                                          \
		snprintf( err.err.message, sizeof( err.err.message ), "[%s:%d] %s", file, line, msg );                         \
		err.err.code = code;                                                                                           \
		return err;                                                                                                    \
	}

/* Macro to check if a Result is OK */
#define RESULT_OK( T, res ) ( ( res )->tag == ( T##_TAG_OK ) )

/* Macro to check if a Result is an error */
#define RESULT_ERR( T, res ) ( ( res )->tag == ( T##_TAG_ERR ) )

/* Macros to create Result instances */
#define MAKE_OK( Type, value ) Type##_Ok( value )

/* Macros to create error Result instances */
#define MAKE_ERR( Type, code, msg ) Type##_Err( code, msg )

/* Macros to create debug error Result instances */
#define MAKE_DERR( Type, code, msg ) Type##_Err_Debug( ( code ), ( msg ), __FILE__, __LINE__ )

/* Macro to unwrap a Result or return a fallback value */
#define UNWRAP_OR_ELSE( Type, res, fallback ) ( ( res ).tag == Type##_TAG_OK ? ( res ).ok.value : ( fallback ) )

#endif // RESULT_H

/**
 * This is free and unencumbered software released into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or distribute
 * this software, either in source code form or as a compiled binary, for any
 * purpose, commercial or non-commercial, and by any means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors of this
 * software dedicate any and all copyright interest in the software to the
 * public domain. We make this dedication for the benefit of the public at large
 * and to the detriment of our heirs and successors. We intend this dedication
 * to be an overt act of relinquishment in perpetuity of all present and future
 * rights to this software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
