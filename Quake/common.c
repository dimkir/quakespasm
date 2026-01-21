/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// common.c -- misc functions used in client and server

#include "quakedef.h"
#include "q_ctype.h"
#include <errno.h>

#include "miniz.h"

static char	*largv[MAX_NUM_ARGVS + 1];
static char	argvdummy[] = " ";

int		safemode;

cvar_t	registered = {"registered","1",CVAR_ROM}; /* set to correct value in COM_CheckRegistered() */
cvar_t	cmdline = {"cmdline","",CVAR_ROM/*|CVAR_SERVERINFO*/}; /* sending cmdline upon CCREQ_RULE_INFO is evil */

static qboolean		com_modified;	// set true if using non-id files

qboolean		fitzmode;

static void COM_Path_f (void);

// if a packfile directory differs from this, it is assumed to be hacked
#define PAK0_COUNT		339	/* id1/pak0.pak - v1.0x */
#define PAK0_CRC_V100		13900	/* id1/pak0.pak - v1.00 */
#define PAK0_CRC_V101		62751	/* id1/pak0.pak - v1.01 */
#define PAK0_CRC_V106		32981	/* id1/pak0.pak - v1.06 */
#define PAK0_CRC	(PAK0_CRC_V106)
#define PAK0_COUNT_V091		308	/* id1/pak0.pak - v0.91/0.92, not supported */
#define PAK0_CRC_V091		28804	/* id1/pak0.pak - v0.91/0.92, not supported */

char	com_token[1024];
int		com_argc;
char	**com_argv;

#define CMDLINE_LENGTH	256		/* johnfitz -- mirrored in cmd.c */
char	com_cmdline[CMDLINE_LENGTH];

qboolean standard_quake = true, rogue, hipnotic;

// this graphic needs to be in the pak file to use registered features
static unsigned short pop[] =
{
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000,
	0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000,
	0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600,
	0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563,
	0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564,
	0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564,
	0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563,
	0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500,
	0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200,
	0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000,
	0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

/*

All of Quake's data access is through a hierchal file system, but the contents
of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all
game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.
This can be overridden with the "-basedir" command line parm to allow code
debugging in a different directory.  The base directory is only used during
filesystem initialization.

The "game directory" is the first tree on the search path and directory that all
generated files (savegames, screenshots, demos, config files) will be saved to.
This can be overridden with the "-game" command line parameter.  The game
directory can never be changed while quake is executing.  This is a precacution
against having a malicious server instruct clients to write files over areas they
shouldn't.

The "cache directory" is only used during development to save network bandwidth,
especially over ISDN / T1 lines.  If there is a cache directory specified, when
a file is found by the normal search path, it will be mirrored into the cache
directory, then opened there.

FIXME:
The file "parms.txt" will be read out of the game directory and appended to the
current command line arguments to allow different games to initialize startup
parms differently.  This could be used to add a "-sspeed 22050" for the high
quality sound edition.  Because they are added at the end, they will not
override an explicit setting on the original command line.

*/

//============================================================================

/*
ClearLink

Initializes a doubly-linked list node to point to itself (creates empty circular list).
This is used when creating new headnodes (sentinel nodes) for doubly-linked lists.
In a circular doubly-linked list, an empty list has the head pointing to itself.
*/
void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

/*
RemoveLink

Removes a node from its doubly-linked list by updating the next/prev pointers
of its neighbors to bypass it. The node itself is not freed (similar to unlinking
a node in a JavaScript linked list, but you need to manually update both directions).
*/
void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

/*
InsertLinkBefore

Inserts node 'l' into the list immediately before the 'before' node.
Updates all four pointers involved: l's next/prev and its neighbors' pointers.
Think of it like splicing a node into a chain by rewiring the connections.
*/
void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

/*
InsertLinkAfter

Inserts node 'l' into the list immediately after the 'after' node.
Updates all four pointers involved: l's next/prev and its neighbors' pointers.
Similar to InsertLinkBefore but in the opposite direction.
*/
void InsertLinkAfter (link_t *l, link_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*

							DYNAMIC VECTORS

These functions implement a growable array (like JavaScript arrays or C++ vectors).
Unlike fixed-size C arrays, these can expand as needed. The header before the data
stores size (current elements) and capacity (allocated space).

*/

/*
Vec_Grow

Ensures the vector has capacity for 'count' additional elements.
If more space is needed, reallocates with 50% growth factor (minimum 16 capacity).
The double-pointer allows modifying the caller's pointer when reallocation occurs.
Note: The actual data starts after a hidden header containing size/capacity.
*/
void Vec_Grow (void **pvec, size_t element_size, size_t count)
{
	vec_header_t header;
	if (*pvec)
		header = VEC_HEADER(*pvec);
	else
		header.size = header.capacity = 0;

	if (header.size + count > header.capacity)
	{
		void *new_buffer;
		size_t total_size;

		header.capacity = header.size + count;
		header.capacity += header.capacity >> 1;
		if (header.capacity < 16)
			header.capacity = 16;
		total_size = sizeof(vec_header_t) + header.capacity * element_size;

		if (*pvec)
			new_buffer = realloc (((vec_header_t*)*pvec) - 1, total_size);
		else
			new_buffer = malloc (total_size);
		if (!new_buffer)
			Sys_Error ("Vec_Grow: failed to allocate %lu bytes\n", (unsigned long)total_size);

		*pvec = 1 + (vec_header_t*)new_buffer;
		VEC_HEADER(*pvec) = header;
	}
}

/*
Vec_Append

Appends 'count' elements from 'data' to the end of the vector.
Automatically grows the vector if needed (calls Vec_Grow internally).
This is like the JavaScript array.push() method but can add multiple elements.
Updates the size in the header after copying the data.
*/
void Vec_Append (void **pvec, size_t element_size, const void *data, size_t count)
{
	if (!count)
		return;
	Vec_Grow (pvec, element_size, count);
	memcpy ((byte *)*pvec + VEC_HEADER(*pvec).size * element_size, data, count * element_size);
	VEC_HEADER(*pvec).size += count;
}

/*
Vec_Clear

Resets the vector to zero elements without freeing the allocated memory.
The capacity remains unchanged, so you can refill it without reallocation.
Similar to calling array.length = 0 in JavaScript.
*/
void Vec_Clear (void **pvec)
{
	if (*pvec)
		VEC_HEADER(*pvec).size = 0;
}

/*
Vec_Free

Frees the vector's memory and sets the pointer to NULL.
Note: We free the header (which starts before the actual data pointer),
then null out the caller's pointer. Always call this to avoid memory leaks.
*/
void Vec_Free (void **pvec)
{
	if (*pvec)
	{
		free(&VEC_HEADER(*pvec));
		*pvec = NULL;
	}
}

/*

					LIBRARY REPLACEMENT FUNCTIONS

These q_ prefixed functions provide portable implementations of common
string/memory functions that may not be available or may behave differently
across platforms (Windows, Linux, macOS). Think of them as polyfills.

*/

/*
q_strcasecmp

Case-insensitive string comparison (like String.toLowerCase() comparison in JS).
Returns 0 if strings are equal (ignoring case), non-zero otherwise.
Note: In C, 0 means success/equal, which is opposite of JavaScript's truthy values.
*/
int q_strcasecmp(const char * s1, const char * s2)
{
	const char * p1 = s1;
	const char * p2 = s2;
	char c1, c2;

	if (p1 == p2)
		return 0;

	do
	{
		c1 = q_tolower (*p1++);
		c2 = q_tolower (*p2++);
		if (c1 == '\0')
			break;
	} while (c1 == c2);

	return (int)(c1 - c2);
}

/*
q_strncasecmp

Case-insensitive string comparison for at most 'n' characters.
Like q_strcasecmp but stops after n characters (useful for prefix matching).
*/
int q_strncasecmp(const char *s1, const char *s2, size_t n)
{
	const char * p1 = s1;
	const char * p2 = s2;
	char c1, c2;

	if (p1 == p2 || n == 0)
		return 0;

	do
	{
		c1 = q_tolower (*p1++);
		c2 = q_tolower (*p2++);
		if (c1 == '\0' || c1 != c2)
			break;
	} while (--n > 0);

	return (int)(c1 - c2);
}

/*
q_strcasestr

Finds first occurrence of 'needle' in 'haystack' (case-insensitive).
Returns pointer to the match or NULL if not found.
Similar to JavaScript's String.indexOf() but returns pointer instead of index.
*/
char *q_strcasestr(const char *haystack, const char *needle)
{
	const size_t len = strlen(needle);

	while (*haystack)
	{
		if (!q_strncasecmp(haystack, needle, len))
			return (char *)haystack;

		++haystack;
	}

	return NULL;
}

/*
q_strlwr

Converts string to lowercase in-place (modifies the original string).
Returns the same pointer for convenience (allows chaining).
Unlike JavaScript strings, C strings are mutable arrays of characters.
*/
char *q_strlwr (char *str)
{
	char	*c;
	c = str;
	while (*c)
	{
		*c = q_tolower(*c);
		c++;
	}
	return str;
}

/*
q_strupr

Converts string to uppercase in-place (modifies the original string).
Returns the same pointer for convenience.
*/
char *q_strupr (char *str)
{
	char	*c;
	c = str;
	while (*c)
	{
		*c = q_toupper(*c);
		c++;
	}
	return str;
}

/*
q_strdup

Creates a malloc'd copy of a string (allocates new memory on the heap).
Caller is responsible for freeing the returned memory later.
In JavaScript, strings are immutable and copied automatically; in C you must do it manually.
*/
char *q_strdup (const char *str)
{
	size_t len = strlen (str) + 1;
	char  *newstr = (char *)malloc (len);
	memcpy (newstr, str, len);
	return newstr;
}

/* platform dependant (v)snprintf function names: */
#if defined(_WIN32)
#define	snprintf_func		_snprintf
#define	vsnprintf_func		_vsnprintf
#else
#define	snprintf_func		snprintf
#define	vsnprintf_func		vsnprintf
#endif

/*
q_vsnprintf

Safe, portable version of vsnprintf (formatted string printing with va_list).
Ensures null-termination and handles platform differences (Windows vs Unix).
The 'va_list' is like JavaScript's 'arguments' object - contains variable arguments.
Returns number of characters that would be written (excluding null terminator).
*/
int q_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	int		ret;

	ret = vsnprintf_func (str, size, format, args);

	if (ret < 0)
		ret = (int)size;
	if (size == 0)	/* no buffer */
		return ret;
	if ((size_t)ret >= size)
		str[size - 1] = '\0';

	return ret;
}

/*
q_snprintf

Safe, portable version of snprintf (like sprintf but with size limit).
Similar to JavaScript template literals or String formatting, but you must
provide a pre-allocated buffer and its size to prevent buffer overflows.
Example: q_snprintf(buf, sizeof(buf), "Player: %s Score: %d", name, score);
*/
int q_snprintf (char *str, size_t size, const char *format, ...)
{
	int		ret;
	va_list		argptr;

	va_start (argptr, format);
	ret = q_vsnprintf (str, size, format, argptr);
	va_end (argptr);

	return ret;
}

/*
Q_memset

Fills memory with a byte value (like Array.fill() but for raw memory).
Optimized version: if alignment allows, fills 4 bytes at a time instead of 1.
This is faster than standard memset on some platforms - a performance trick
from the 90s that may still help on some systems.
*/
void Q_memset (void *dest, int fill, size_t count)
{
	size_t		i;

	if ( (((uintptr_t)dest | count) & 3) == 0)
	{
		count >>= 2;
		fill = fill | (fill<<8) | (fill<<16) | (fill<<24);
		for (i = 0; i < count; i++)
			((int *)dest)[i] = fill;
	}
	else
		for (i = 0; i < count; i++)
			((byte *)dest)[i] = fill;
}

/*
Q_memcpy

Copies memory from src to dest (like Object.assign() but for raw bytes).
Optimized to copy 4-byte chunks when possible instead of byte-by-byte.
WARNING: Unlike JavaScript, this doesn't handle overlapping memory regions.
Use memmove() if source and destination might overlap.
*/
void Q_memcpy (void *dest, const void *src, size_t count)
{
	size_t		i;

	if (( ( (uintptr_t)dest | (uintptr_t)src | count) & 3) == 0)
	{
		count >>= 2;
		for (i = 0; i < count; i++)
			((int *)dest)[i] = ((int *)src)[i];
	}
	else
		for (i = 0; i < count; i++)
			((byte *)dest)[i] = ((byte *)src)[i];
}

/*
Q_memcmp

Compares two memory regions byte-by-byte.
Returns 0 if identical, -1 if different.
Note: This is simpler than standard memcmp (which returns the difference value).
JavaScript equivalent: comparing two TypedArrays element by element.
*/
int Q_memcmp (const void *m1, const void *m2, size_t count)
{
	while(count)
	{
		count--;
		if (((byte *)m1)[count] != ((byte *)m2)[count])
			return -1;
	}
	return 0;
}

/*
Q_strcpy

Copies a null-terminated string from src to dest.
WARNING: No bounds checking! Dest must be large enough.
In JavaScript, strings are immutable and auto-managed. In C, you must
manually copy them and ensure sufficient space to avoid buffer overflows.
*/
void Q_strcpy (char *dest, const char *src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}

/*
Q_strncpy

Copies up to 'count' characters from src to dest, null-terminates if space allows.
Safer than Q_strcpy because it limits the copy length.
Note: May not null-terminate if src is >= count chars (unlike strncpy standard behavior).
*/
void Q_strncpy (char *dest, const char *src, int count)
{
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
}

/*
Q_strlen

Returns the length of a null-terminated string (like String.length in JS).
Counts characters until hitting the '\0' terminator.
Every C string must end with '\0' - that's how we know where it ends.
*/
int Q_strlen (const char *str)
{
	int		count;

	count = 0;
	while (str[count])
		count++;

	return count;
}

/*
Q_strrchr

Finds the LAST occurrence of character 'c' in string 's'.
Returns pointer to that character, or NULL if not found.
Like JavaScript's String.lastIndexOf() but returns a pointer instead of index.
Useful for finding file extensions (last '.' in a filename).
*/
char *Q_strrchr(const char *s, char c)
{
	int len = Q_strlen(s);
	s += len;
	while (len--)
	{
		if (*--s == c)
			return (char *)s;
	}
	return NULL;
}

/*
Q_strcat

Appends src string to the end of dest string (like string concatenation in JS).
WARNING: No bounds checking! Dest must have enough space for both strings.
In JavaScript you'd do: dest += src. In C, you modify dest in-place.
*/
void Q_strcat (char *dest, const char *src)
{
	dest += Q_strlen(dest);
	Q_strcpy (dest, src);
}

/*
Q_strcmp

Compares two strings for exact equality (case-sensitive).
Returns 0 if equal, -1 if different.
Note: In C, 0 means true/success. In JS you'd use === or String.localeCompare().
*/
int Q_strcmp (const char *s1, const char *s2)
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

/*
Q_strncmp

Compares up to 'count' characters of two strings.
Returns 0 if equal, -1 if different.
Useful for checking string prefixes: Q_strncmp(str, "prefix", 6)
*/
int Q_strncmp (const char *s1, const char *s2, int count)
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

/*
Q_atoi

Converts a string to an integer (like parseInt() in JavaScript).
Supports:
 - Negative numbers (leading '-')
 - Hexadecimal (0x prefix)
 - Character literals (single quote: 'A' returns 65)
 - Decimal numbers
Ignores leading whitespace.
*/
int Q_atoi (const char *str)
{
	int		val;
	int		sign;
	int		c;

	while (q_isspace (*str))
		++str;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}

	return 0;
}


/*
Q_atof

Converts a string to a floating-point number (like parseFloat() in JavaScript).
Supports:
 - Negative numbers (leading '-')
 - Hexadecimal floats (0x prefix)
 - Character literals (single quote)
 - Decimal numbers with decimal point
Ignores leading whitespace. Handles the fractional part by tracking decimal position.
*/
float Q_atof (const char *str)
{
	double		val;
	int		sign;
	int		c;
	int	decimal, total;

	while (q_isspace (*str))
		++str;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	decimal = -1;
	total = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
			break;
		val = val*10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}

	return val*sign;
}

/*

					BYTE ORDER FUNCTIONS

These functions handle endianness - the order bytes are stored in multi-byte values.
Big-endian (BE): most significant byte first (like reading left-to-right)
Little-endian (LE): least significant byte first (x86/x64 CPUs use this)

Quake's network protocol and file formats use little-endian, so on big-endian
machines (old PowerPC Macs, some game consoles), we need to swap bytes.
JavaScript hides this complexity with DataView, but C requires manual handling.

*/

qboolean	host_bigendian;

short	(*BigShort) (short l);
short	(*LittleShort) (short l);
int	(*BigLong) (int l);
int	(*LittleLong) (int l);
float	(*BigFloat) (float l);
float	(*LittleFloat) (float l);

/*
ShortSwap

Swaps the byte order of a 16-bit short (2 bytes).
Example: 0x1234 becomes 0x3412
Extracts each byte using bit masking (&255), then reassembles in reverse order.
Think of it like reversing a 2-element array, but at the byte level.
*/
short ShortSwap (short l)
{
	byte	b1, b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

/*
ShortNoSwap

Returns the short unchanged (no-op function).
Used as a function pointer when no swapping is needed.
Having this allows using the same code path regardless of platform endianness.
*/
short ShortNoSwap (short l)
{
	return l;
}

/*
LongSwap

Swaps the byte order of a 32-bit integer (4 bytes).
Example: 0x12345678 becomes 0x78563412
Extracts all 4 bytes, then reassembles them in reverse order using bit shifts.
*/
int LongSwap (int l)
{
	byte	b1, b2, b3, b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

/*
LongNoSwap

Returns the integer unchanged (no-op function).
Function pointer variant for platforms that don't need byte swapping.
*/
int LongNoSwap (int l)
{
	return l;
}

/*
FloatSwap

Swaps the byte order of a 32-bit float.
Uses a union (C's way to reinterpret memory) to access the float as bytes,
then reverses the 4 bytes. JavaScript doesn't expose this level of control.
Union lets us view the same memory as either a float or an array of bytes.
*/
float FloatSwap (float f)
{
	union
	{
		float	f;
		byte	b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

/*
FloatNoSwap

Returns the float unchanged (no-op function).
Function pointer variant for platforms that don't need byte swapping.
*/
float FloatNoSwap (float f)
{
	return f;
}

/*

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors

These functions serialize data into binary network messages (like writing to
a ByteBuffer or ArrayBuffer in JavaScript). The 'sizebuf_t' is a growable
binary buffer, and these functions write different data types into it.
All multi-byte values are written in little-endian format for network compatibility.

*/

//
// writing functions
//

/*
MSG_WriteChar

Writes a signed char (-128 to 127) to the message buffer.
In debug mode (PARANOID), validates the range. Gets buffer space and writes 1 byte.
Think of this like DataView.setInt8() in JavaScript.
*/
void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID
	if (c < -128 || c > 127)
		Sys_Error ("MSG_WriteChar: range error");
#endif

	buf = (byte *) SZ_GetSpace (sb, 1);
	buf[0] = c;
}

/*
MSG_WriteByte

Writes an unsigned byte (0 to 255) to the message buffer.
Most common write operation - used for commands, flags, small values.
Like DataView.setUint8() in JavaScript.
*/
void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID
	if (c < 0 || c > 255)
		Sys_Error ("MSG_WriteByte: range error");
#endif

	buf = (byte *) SZ_GetSpace (sb, 1);
	buf[0] = c;
}

/*
MSG_WriteShort

Writes a 16-bit short (-32768 to 32767) in little-endian format.
Manually extracts low and high bytes - low byte first (little-endian).
JavaScript equivalent: DataView.setInt16(offset, value, true) where true = little-endian.
*/
void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Sys_Error ("MSG_WriteShort: range error");
#endif

	buf = (byte *) SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

/*
MSG_WriteLong

Writes a 32-bit integer in little-endian format (4 bytes, lowest byte first).
Manually extracts each byte using bit shifts and masking.
JavaScript: DataView.setInt32(offset, value, true).
*/
void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte	*buf;

	buf = (byte *) SZ_GetSpace (sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

/*
MSG_WriteFloat

Writes a 32-bit floating point number.
Uses a union trick to reinterpret the float's bits as an integer,
converts to little-endian, then writes 4 bytes.
JavaScript: DataView.setFloat32(offset, value, true).
*/
void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float	f;
		int	l;
	} dat;

	dat.f = f;
	dat.l = LittleLong (dat.l);

	SZ_Write (sb, &dat.l, 4);
}

/*
MSG_WriteString

Writes a null-terminated string (C-string) to the buffer.
Writes all characters plus the '\0' terminator.
JavaScript doesn't have null-terminated strings - the '\0' marks the end in C.
*/
void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, s, Q_strlen(s)+1);
}

//johnfitz -- original behavior, 13.3 fixed point coords, max range +-4096
void MSG_WriteCoord16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, Q_rint(f*8));
}

//johnfitz -- 16.8 fixed point coords, max range +-32768
void MSG_WriteCoord24 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, f);
	MSG_WriteByte (sb, (int)(f*255)%255);
}

//johnfitz -- 32-bit float coords
void MSG_WriteCoord32f (sizebuf_t *sb, float f)
{
	MSG_WriteFloat (sb, f);
}

void MSG_WriteCoord (sizebuf_t *sb, float f, unsigned int flags)
{
	if (flags & PRFL_FLOATCOORD)
		MSG_WriteFloat (sb, f);
	else if (flags & PRFL_INT32COORD)
		MSG_WriteLong (sb, Q_rint (f * 16));
	else if (flags & PRFL_24BITCOORD)
		MSG_WriteCoord24 (sb, f);
	else MSG_WriteCoord16 (sb, f);
}

void MSG_WriteAngle (sizebuf_t *sb, float f, unsigned int flags)
{
	if (flags & PRFL_FLOATANGLE)
		MSG_WriteFloat (sb, f);
	else if (flags & PRFL_SHORTANGLE)
		MSG_WriteShort (sb, Q_rint(f * 65536.0 / 360.0) & 65535);
	else MSG_WriteByte (sb, Q_rint(f * 256.0 / 360.0) & 255); //johnfitz -- use Q_rint instead of (int)	}
}

//johnfitz -- for PROTOCOL_FITZQUAKE
void MSG_WriteAngle16 (sizebuf_t *sb, float f, unsigned int flags)
{
	if (flags & PRFL_FLOATANGLE)
		MSG_WriteFloat (sb, f);
	else MSG_WriteShort (sb, Q_rint(f * 65536.0 / 360.0) & 65535);
}
//johnfitz

//
// reading functions
//
int		msg_readcount;
qboolean	msg_badread;

/*
MSG_BeginReading

Resets the read position to the start of the message buffer.
Must be called before reading any data from a new message.
Similar to resetting a file pointer or ArrayBuffer read offset to 0.
*/
void MSG_BeginReading (void)
{
	msg_readcount = 0;
	msg_badread = false;
}

/*
MSG_ReadChar

Reads a signed char (-128 to 127) from the message.
Returns -1 and sets msg_badread flag if trying to read past the end.
The cast to 'signed char' is important - ensures sign extension.
*/
// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar (void)
{
	int	c;

	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (signed char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

/*
MSG_ReadByte

Reads an unsigned byte (0-255) from the message.
Most common read operation. Cast to unsigned char prevents sign extension.
Like DataView.getUint8() in JavaScript.
*/
int MSG_ReadByte (void)
{
	int	c;

	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (unsigned char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadShort (void)
{
	int	c;

	if (msg_readcount+2 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (short)(net_message.data[msg_readcount]
			+ (net_message.data[msg_readcount+1]<<8));

	msg_readcount += 2;

	return c;
}

int MSG_ReadLong (void)
{
	int	c;

	if (msg_readcount+4 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = net_message.data[msg_readcount]
			+ (net_message.data[msg_readcount+1]<<8)
			+ (net_message.data[msg_readcount+2]<<16)
			+ (net_message.data[msg_readcount+3]<<24);

	msg_readcount += 4;

	return c;
}

float MSG_ReadFloat (void)
{
	union
	{
		byte	b[4];
		float	f;
		int	l;
	} dat;

	dat.b[0] = net_message.data[msg_readcount];
	dat.b[1] = net_message.data[msg_readcount+1];
	dat.b[2] = net_message.data[msg_readcount+2];
	dat.b[3] = net_message.data[msg_readcount+3];
	msg_readcount += 4;

	dat.l = LittleLong (dat.l);

	return dat.f;
}

const char *MSG_ReadString (void)
{
	static char	string[2048];
	int		c;
	size_t		l;

	l = 0;
	do
	{
		c = MSG_ReadByte ();
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string) - 1);

	string[l] = 0;

	return string;
}

//johnfitz -- original behavior, 13.3 fixed point coords, max range +-4096
float MSG_ReadCoord16 (void)
{
	return MSG_ReadShort() * (1.0/8);
}

//johnfitz -- 16.8 fixed point coords, max range +-32768
float MSG_ReadCoord24 (void)
{
	return MSG_ReadShort() + MSG_ReadByte() * (1.0/255);
}

//johnfitz -- 32-bit float coords
float MSG_ReadCoord32f (void)
{
	return MSG_ReadFloat();
}

float MSG_ReadCoord (unsigned int flags)
{
	if (flags & PRFL_FLOATCOORD)
		return MSG_ReadFloat ();
	else if (flags & PRFL_INT32COORD)
		return MSG_ReadLong () * (1.0 / 16.0);
	else if (flags & PRFL_24BITCOORD)
		return MSG_ReadCoord24 ();
	else return MSG_ReadCoord16 ();
}

float MSG_ReadAngle (unsigned int flags)
{
	if (flags & PRFL_FLOATANGLE)
		return MSG_ReadFloat ();
	else if (flags & PRFL_SHORTANGLE)
		return MSG_ReadShort () * (360.0 / 65536);
	else return MSG_ReadChar () * (360.0 / 256);
}

//johnfitz -- for PROTOCOL_FITZQUAKE
float MSG_ReadAngle16 (unsigned int flags)
{
	if (flags & PRFL_FLOATANGLE)
		return MSG_ReadFloat ();	// make sure
	else return MSG_ReadShort () * (360.0 / 65536);
}
//johnfitz

//===========================================================================

/*
SZ_Alloc

Allocates and initializes a sizebuf_t (size buffer) structure.
Allocates from the hunk (Quake's custom memory allocator) with minimum 256 bytes.
The sizebuf_t is like a growable ByteBuffer - tracks current size and max capacity.
Used for network messages and other binary data serialization.
*/
void SZ_Alloc (sizebuf_t *buf, int startsize)
{
	if (startsize < 256)
		startsize = 256;
	buf->data = (byte *) Hunk_AllocName (startsize, "sizebuf");
	buf->maxsize = startsize;
	buf->cursize = 0;
}


void SZ_Free (sizebuf_t *buf)
{
//	Z_Free (buf->data);
//	buf->data = NULL;
//	buf->maxsize = 0;
	buf->cursize = 0;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
}

/*
SZ_GetSpace

Reserves space in the buffer and returns a pointer to it.
Grows the buffer if needed (if allowoverflow is set), otherwise errors.
This is how you get a writable region before writing data.
Returns pointer to the reserved space and updates cursize.
*/
void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void	*data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Host_Error ("SZ_GetSpace: overflow without allowoverflow set"); // ericw -- made Host_Error to be less annoying

		if (length > buf->maxsize)
			Sys_Error ("SZ_GetSpace: %i is > full buffer size", length);

		buf->overflowed = true;
		Con_Printf ("SZ_GetSpace: overflow\n");
		SZ_Clear (buf);
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

/*
SZ_Write

Writes binary data to the buffer.
Simple wrapper: gets space then copies data using Q_memcpy.
Like writing to a ByteBuffer or pushing bytes to an array.
*/
void SZ_Write (sizebuf_t *buf, const void *data, int length)
{
	Q_memcpy (SZ_GetSpace(buf,length),data,length);
}

/*
SZ_Print

Appends a string to the buffer, handling null termination intelligently.
If buffer already has a trailing null, overwrites it (no double-null).
Otherwise appends the string with its null terminator.
Useful for building command strings piece by piece.
*/
void SZ_Print (sizebuf_t *buf, const char *data)
{
	int		len = Q_strlen(data) + 1;

	if (buf->data[buf->cursize-1])
	{	/* no trailing 0 */
		Q_memcpy ((byte *)SZ_GetSpace(buf, len  )  , data, len);
	}
	else
	{	/* write over trailing 0 */
		Q_memcpy ((byte *)SZ_GetSpace(buf, len-1)-1, data, len);
	}
}


//============================================================================

/*
COM_SkipPath

Returns a pointer to the filename part of a path (skips directories).
Example: "maps/e1m1.bsp" returns pointer to "e1m1.bsp"
Finds the last '/' character and returns everything after it.
JavaScript equivalent: path.split('/').pop() or path.basename()
*/
const char *COM_SkipPath (const char *pathname)
{
	const char	*last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname == '/')
			last = pathname + 1;
		pathname++;
	}
	return last;
}

/*
COM_StripExtension

Removes the file extension from a path.
Example: "player.mdl" becomes "player"
Searches backwards for '.', ensuring it's not in a parent directory name.
Can work in-place (in == out) or copy to a new buffer.
JavaScript: path.substring(0, path.lastIndexOf('.'))
*/
void COM_StripExtension (const char *in, char *out, size_t outsize)
{
	int	length;

	if (!*in)
	{
		*out = '\0';
		return;
	}
	if (in != out)	/* copy when not in-place editing */
		q_strlcpy (out, in, outsize);
	length = (int)strlen(out) - 1;
	while (length > 0 && out[length] != '.')
	{
		--length;
		if (out[length] == '/' || out[length] == '\\')
			return;	/* no extension */
	}
	if (length > 0)
		out[length] = '\0';
}

/*
COM_FileGetExtension

Returns a pointer to the file extension (without the dot).
Example: "textures/wall.tga" returns "tga"
Returns empty string "" if no extension found.
Never returns NULL - safe to use in comparisons without null checks.
*/
const char *COM_FileGetExtension (const char *in)
{
	const char	*src;
	size_t		len;

	len = strlen(in);
	if (len < 2)	/* nothing meaningful */
		return "";

	src = in + len - 1;
	while (src != in && src[-1] != '.')
		src--;
	if (src == in || strchr(src, '/') != NULL || strchr(src, '\\') != NULL)
		return "";	/* no extension, or parent directory has a dot */

	return src;
}

/*
COM_ExtractExtension
*/
void COM_ExtractExtension (const char *in, char *out, size_t outsize)
{
	const char *ext = COM_FileGetExtension (in);
	if (! *ext)
		*out = '\0';
	else
		q_strlcpy (out, ext, outsize);
}

/*
COM_FileBase

Extracts just the filename without path or extension.
Example: "maps/dm/face.bsp" becomes "face"
Finds the last slash and last dot, extracts what's between them.
Returns "?model?" if the result would be invalid.
*/
void COM_FileBase (const char *in, char *out, size_t outsize)
{
	const char	*dot, *slash, *s;

	s = in;
	slash = in;
	dot = NULL;
	while (*s)
	{
		if (*s == '/' || *s == '\\')
			slash = s + 1;
		if (*s == '.')
			dot = s;
		s++;
	}
	if (dot == NULL)
		dot = s;

	if (dot - slash < 2)
		q_strlcpy (out, "?model?", outsize);
	else
	{
		size_t	len = dot - slash;
		if (len >= outsize)
			len = outsize - 1;
		memcpy (out, slash, len);
		out[len] = '\0';
	}
}

/*
COM_DefaultExtension
if path doesn't have a .EXT, append extension
(extension should include the leading ".")
*/
#if 0 /* can be dangerous */
void COM_DefaultExtension (char *path, const char *extension, size_t len)
{
	char	*src;

	if (!*path) return;
	src = path + strlen(path) - 1;

	while (*src != '/' && *src != '\\' && src != path)
	{
		if (*src == '.')
			return; // it has an extension
		src--;
	}

	q_strlcat(path, extension, len);
}
#endif

/*
COM_AddExtension
if path extension doesn't match .EXT, append it
(extension should include the leading ".")
*/
void COM_AddExtension (char *path, const char *extension, size_t len)
{
	if (strcmp(COM_FileGetExtension(path), extension + 1) != 0)
		q_strlcat(path, extension, len);
}


/*
COM_ParseEx

Parse a token out of a string

Extracts the next word or token from a text string.
Handles:
 - Whitespace skipping
 - C++ style comments (slash-slash and slash-star types)
 - Quoted strings
 - Single character tokens: braces, parens, quote, colon
 - Regular words (anything else up to whitespace)

The mode argument controls how overflow is handled:
- CPE_NOTRUNC: return NULL (abort parsing)
- CPE_ALLOWTRUNC: truncate com_token (ignore the extra characters in this token)

Stores result in global com_token buffer. Returns pointer to remaining text.
This is like a manual tokenizer - JavaScript has String.split() and regex for this.
*/
const char *COM_ParseEx (const char *data, cpe_mode mode)
{
	int		c;
	int		len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;	// end of file
		data++;
	}

// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

// skip /*..*/ comments
	if (c == '/' && data[1] == '*')
	{
		data += 2;
		while (*data && !(*data == '*' && data[1] == '/'))
			data++;
		if (*data)
			data += 2;
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if ((c = *data) != 0)
				++data;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			if (len < Q_COUNTOF(com_token) - 1)
				com_token[len++] = c;
			else if (mode == CPE_NOTRUNC)
				return NULL;
		}
	}

// parse single characters
	if (c == '{' || c == '}'|| c == '('|| c == ')' || c == '\'' || c == ':')
	{
		if (len < Q_COUNTOF(com_token) - 1)
			com_token[len++] = c;
		else if (mode == CPE_NOTRUNC)
			return NULL;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		if (len < Q_COUNTOF(com_token) - 1)
			com_token[len++] = c;
		else if (mode == CPE_NOTRUNC)
			return NULL;
		data++;
		c = *data;
		/* commented out the check for ':' so that ip:port works */
		if (c == '{' || c == '}'|| c == '('|| c == ')' || c == '\''/* || c == ':' */)
			break;
	} while (c > 32);

	com_token[len] = 0;
	return data;
}


/*
COM_Parse

Parse a token out of a string

Simple wrapper around COM_ParseEx that returns NULL on overflow.
This is the "safe" version that aborts if token is too long.
Return NULL in case of overflow
*/
const char *COM_Parse (const char *data)
{
	return COM_ParseEx (data, CPE_NOTRUNC);
}


/*
COM_CheckParm

Searches command-line arguments for a specific parameter.
Returns the position (1 to argc-1) where the parameter appears, or 0 if not present.

Example: If launched with "quake -window -width 1024"
  COM_CheckParm("-window") returns its position
  COM_CheckParm("-nosound") returns 0

JavaScript equivalent: process.argv.indexOf(param)
*/
int COM_CheckParm (const char *parm)
{
	int		i;

	for (i = 1; i < com_argc; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP sometimes clears appkit vars.
		if (!Q_strcmp (parm,com_argv[i]))
			return i;
	}

	return 0;
}

/*
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.

Quake had shareware (free, limited) and registered (paid, full) versions.
This checks for the registered version by looking for a specific graphic file
(gfx/pop.lmp) and verifying its checksum matches the original.
Prevents piracy by ensuring you have the real game data.
Also prevents loading mods without owning the full game.
*/
static void COM_CheckRegistered (void)
{
	int		h;
	unsigned short	check[128];
	int		i;

	COM_OpenFile("gfx/pop.lmp", &h, NULL);

	if (h == -1)
	{
		Cvar_SetROM ("registered", "0");
		Con_Printf ("Playing shareware version.\n");
		if (com_modified)
			Sys_Error ("You must have the registered version to use modified games.\n\n"
				   "Basedir is: %s\n\n"
				   "Check that this has an " GAMENAME " subdirectory containing pak0.pak and pak1.pak, "
				   "or use the -basedir command-line option to specify another directory.",
				   com_basedir);
		return;
	}

	i = Sys_FileRead (h, check, sizeof(check));
	COM_CloseFile (h);
	if (i != (int) sizeof(check))
		goto corrupt;

	for (i = 0; i < 128; i++)
	{
		if (pop[i] != (unsigned short)BigShort (check[i]))
		{ corrupt:
			Sys_Error ("Corrupted data file.");
		}
	}

	for (i = 0; com_cmdline[i]; i++)
	{
		if (com_cmdline[i]!= ' ')
			break;
	}

	Cvar_SetROM ("cmdline", &com_cmdline[i]);
	Cvar_SetROM ("registered", "1");
	Con_Printf ("Playing registered version.\n");
}


/*
COM_InitArgv

Initializes the command-line arguments system.

Reconstructs the full command line into a single string for the cmdline cvar.
Stores argv pointers for later lookup via COM_CheckParm.
Detects special flags like -safe, -rogue, -hipnotic, -quoth.

These command-line args control which game/mod to load:
  -rogue: Dissolution of Eternity mission pack
  -hipnotic: Scourge of Armagon mission pack  
  -quoth: Quoth mod

JavaScript equivalent: process.argv parsing in Node.js.
*/
void COM_InitArgv (int argc, char **argv)
{
	int		i, j, n;

// reconstitute the command line for the cmdline externally visible cvar
	n = 0;

	for (j = 0; (j<MAX_NUM_ARGVS) && (j< argc); j++)
	{
		i = 0;

		while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i])
		{
			com_cmdline[n++] = argv[j][i++];
		}

		if (n < (CMDLINE_LENGTH - 1))
			com_cmdline[n++] = ' ';
		else
			break;
	}

	if (n > 0 && com_cmdline[n-1] == ' ')
		com_cmdline[n-1] = 0; //johnfitz -- kill the trailing space

	Con_Printf("Command line: %s\n", com_cmdline);

	for (com_argc = 0; (com_argc < MAX_NUM_ARGVS) && (com_argc < argc); com_argc++)
	{
		largv[com_argc] = argv[com_argc];
		if (!Q_strcmp ("-safe", argv[com_argc]))
			safemode = 1;
	}

	largv[com_argc] = argvdummy;
	com_argv = largv;

	if (COM_CheckParm ("-rogue"))
	{
		rogue = true;
		standard_quake = false;
	}

	if (COM_CheckParm ("-hipnotic") || COM_CheckParm ("-quoth")) //johnfitz -- "-quoth" support
	{
		hipnotic = true;
		standard_quake = false;
	}
}

/*
COM_Init

Initializes the common module by detecting the system's endianness.
Sets up function pointers for byte swapping based on the CPU architecture.
Uses a clever trick: stores 0x12345678 and checks which byte appears first in memory.
- If first byte is 0x78: little-endian (x86/x64)
- If first byte is 0x12: big-endian (PowerPC, some ARM)
- If first byte is 0x34: PDP-endian (ancient, unsupported)
JavaScript hides this - DataView handles it automatically.
*/
void COM_Init (void)
{
	int	i = 0x12345678;
		/*    U N I X */

	/*
	BE_ORDER:  12 34 56 78
		   U  N  I  X

	LE_ORDER:  78 56 34 12
		   X  I  N  U

	PDP_ORDER: 34 12 78 56
		   N  U  X  I
	*/
	if ( *(char *)&i == 0x12 )
		host_bigendian = true;
	else if ( *(char *)&i == 0x78 )
		host_bigendian = false;
	else /* if ( *(char *)&i == 0x34 ) */
		Sys_Error ("Unsupported endianism.");

	if (host_bigendian)
	{
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}
	else /* assumed LITTLE_ENDIAN. */
	{
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}

	if (COM_CheckParm("-fitz"))
		fitzmode = true;
}


/*
va

Varargs printf into a temp buffer - cycles between 4 static buffers.

This is a convenience function for formatting strings without manually managing buffers.
Cycles through 4 buffers so you can use va() multiple times in one function call:
  Printf(\"%s and %s\", va(\"x=%d\", x), va(\"y=%d\", y));  // Works!

IMPORTANT: The returned pointer is only valid until the 5th call to va(),
when it wraps around and reuses the buffer. Don't store the pointer long-term.

JavaScript equivalent: using template literals: `Player: ${name} Score: ${score}`
But in C we need manual formatting and this buffer management.

FIXME: make this buffer size safe someday
*/
#define	VA_NUM_BUFFS	4
#if (MAX_OSPATH >= 1024)
#define	VA_BUFFERLEN	MAX_OSPATH
#else
#define	VA_BUFFERLEN	1024
#endif

static char *get_va_buffer(void)
{
	static char va_buffers[VA_NUM_BUFFS][VA_BUFFERLEN];
	static int buffer_idx = 0;
	buffer_idx = (buffer_idx + 1) & (VA_NUM_BUFFS - 1);
	return va_buffers[buffer_idx];
}

char *va (const char *format, ...)
{
	va_list		argptr;
	char		*va_buf;

	va_buf = get_va_buffer ();
	va_start (argptr, format);
	q_vsnprintf (va_buf, VA_BUFFERLEN, format, argptr);
	va_end (argptr);

	return va_buf;
}

/*

QUAKE FILESYSTEM

*/

int	com_filesize;


//
// on-disk pakfile
//
typedef struct
{
	char	name[56];
	int		filepos, filelen;
} dpackfile_t;

typedef struct
{
	char	id[4];
	int		dirofs;
	int		dirlen;
} dpackheader_t;

#define MAX_FILES_IN_PACK	2048

char	com_gamedir[MAX_OSPATH];
char	com_basedir[MAX_OSPATH];
int	file_from_pak;		// ZOID: global indicating that file came from a pak

searchpath_t	*com_searchpaths;
searchpath_t	*com_base_searchpaths;

/*
COM_Path_f
*/
static void COM_Path_f (void)
{
	searchpath_t	*s;

	Con_Printf ("Current search path:\n");
	for (s = com_searchpaths; s; s = s->next)
	{
		if (s->pack)
		{
			Con_Printf ("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
		}
		else
			Con_Printf ("%s\n", s->filename);
	}
}

/*
COM_WriteFile

The filename will be prefixed by the current game directory
*/
void COM_WriteFile (const char *filename, const void *data, int len)
{
	int		handle;
	char	name[MAX_OSPATH];

	Sys_mkdir (com_gamedir); //johnfitz -- if we've switched to a nonexistant gamedir, create it now so we don't crash

	q_snprintf (name, sizeof(name), "%s/%s", com_gamedir, filename);

	handle = Sys_FileOpenWrite (name);
	if (handle == -1)
	{
		Sys_Printf ("COM_WriteFile: failed on %s\n", name);
		return;
	}

	Sys_Printf ("COM_WriteFile: %s\n", name);
	Sys_FileWrite (handle, data, len);
	Sys_FileClose (handle);
}

/*
COM_CreatePath
*/
void COM_CreatePath (char *path)
{
	char	*ofs;

	for (ofs = path + 1; *ofs; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_mkdir (path);
			*ofs = '/';
		}
	}
}

/*
COM_filelength
*/
long COM_filelength (FILE *f)
{
	long		pos, end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

/*
COM_FindFile

Finds the file in the search path.
Sets com_filesize and one of handle or file
If neither of file or handle is set, this
can be used for detecting a file's presence.

This searches through the virtual filesystem - checking both:
1. PAK files (Quake's archive format, like ZIP files)
2. Actual files on disk

Searches in order of search paths (mods override base game).
Returns filesize on success, -1 on failure.
JavaScript games usually fetch from HTTP - Quake reads from local disk/archives.
*/
static int COM_FindFile (const char *filename, int *handle, FILE **file,
							unsigned int *path_id)
{
	searchpath_t	*search;
	char		netpath[MAX_OSPATH];
	pack_t		*pak;
	int		i;

	if (file && handle)
		Sys_Error ("COM_FindFile: both handle and file set");

	file_from_pak = 0;

//
// search through the path, one element at a time
//
	for (search = com_searchpaths; search; search = search->next)
	{
		if (search->pack)	/* look through all the pak file elements */
		{
			pak = search->pack;
			for (i = 0; i < pak->numfiles; i++)
			{
				if (strcmp(pak->files[i].name, filename) != 0)
					continue;
				// found it!
				com_filesize = pak->files[i].filelen;
				file_from_pak = 1;
				if (path_id)
					*path_id = search->path_id;
				if (handle)
				{
					*handle = pak->handle;
					Sys_FileSeek (pak->handle, pak->files[i].filepos);
					return com_filesize;
				}
				else if (file)
				{ /* open a new file on the pakfile */
					*file = fopen (pak->filename, "rb");
					if (*file)
						fseek (*file, pak->files[i].filepos, SEEK_SET);
					return com_filesize;
				}
				else /* for COM_FileExists() */
				{
					return com_filesize;
				}
			}
		}
		else	/* check a file in the directory tree */
		{
			if (!registered.value)
			{ /* if not a registered version, don't ever go beyond base */
				if ( strchr (filename, '/') || strchr (filename,'\\'))
					continue;
			}

			q_snprintf (netpath, sizeof(netpath), "%s/%s",search->filename, filename);
			if (! (Sys_FileType(netpath) & FS_ENT_FILE))
				continue;

			if (path_id)
				*path_id = search->path_id;
			if (handle)
			{
				com_filesize = Sys_FileOpenRead (netpath, &i);
				*handle = i;
				return com_filesize;
			}
			else if (file)
			{
				*file = fopen (netpath, "rb");
				com_filesize = (*file == NULL) ? -1 : COM_filelength (*file);
				return com_filesize;
			}
			else
			{
				return 0; /* dummy valid value for COM_FileExists() */
			}
		}
	}

	if (strcmp(COM_FileGetExtension(filename), "pcx") != 0
		&& strcmp(COM_FileGetExtension(filename), "tga") != 0
		&& strcmp(COM_FileGetExtension(filename), "lit") != 0
		&& strcmp(COM_FileGetExtension(filename), "vis") != 0
		&& strcmp(COM_FileGetExtension(filename), "ent") != 0)
		Con_DPrintf ("FindFile: can't find %s\n", filename);
	else	Con_DPrintf2("FindFile: can't find %s\n", filename);

	if (handle)
		*handle = -1;
	if (file)
		*file = NULL;
	com_filesize = -1;
	return com_filesize;
}


/*
COM_FileExists

Returns whether the file is found in the quake filesystem.
Simple wrapper around COM_FindFile that returns boolean.
Checks both PAK files and real filesystem.
*/
qboolean COM_FileExists (const char *filename, unsigned int *path_id)
{
	int ret = COM_FindFile (filename, NULL, NULL, path_id);
	return (ret == -1) ? false : true;
}

/*
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
*/
int COM_OpenFile (const char *filename, int *handle, unsigned int *path_id)
{
	return COM_FindFile (filename, handle, NULL, path_id);
}

/*
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
*/
int COM_FOpenFile (const char *filename, FILE **file, unsigned int *path_id)
{
	return COM_FindFile (filename, NULL, file, path_id);
}

/*
COM_CloseFile

If it is a pak file handle, don't really close it
*/
void COM_CloseFile (int h)
{
	searchpath_t	*s;

	for (s = com_searchpaths; s; s = s->next)
		if (s->pack && s->pack->handle == h)
			return;

	Sys_FileClose (h);
}


/*
COM_LoadFile

Filename are relative to the quake directory.
Always appends a 0 byte.

Loads a file into memory using different allocation strategies:
- LOADFILE_HUNK: permanent allocation (level data)
- LOADFILE_TEMPHUNK: temporary allocation (freed when level changes)
- LOADFILE_ZONE: general purpose heap
- LOADFILE_CACHE: can be freed and reloaded as needed
- LOADFILE_STACK: use provided buffer or temp hunk if too large
- LOADFILE_MALLOC: standard malloc (caller must free)

Returns pointer to loaded data with null terminator added.
JavaScript equivalent: fetch() or fs.readFile() - but with manual memory management.
*/
#define	LOADFILE_ZONE		0
#define	LOADFILE_HUNK		1
#define	LOADFILE_TEMPHUNK	2
#define	LOADFILE_CACHE		3
#define	LOADFILE_STACK		4
#define	LOADFILE_MALLOC		5

static byte	*loadbuf;
static cache_user_t *loadcache;
static int	loadsize;

byte *COM_LoadFile (const char *path, int usehunk, unsigned int *path_id)
{
	int		h;
	byte	*buf;
	char	base[32];
	int	len, nread;

	buf = NULL;	// quiet compiler warning

// look for it in the filesystem or pack files
	len = COM_OpenFile (path, &h, path_id);
	if (h == -1)
		return NULL;

// extract the filename base name for hunk tag
	COM_FileBase (path, base, sizeof(base));

	switch (usehunk)
	{
	case LOADFILE_HUNK:
		buf = (byte *) Hunk_AllocName (len+1, base);
		break;
	case LOADFILE_TEMPHUNK:
		buf = (byte *) Hunk_TempAlloc (len+1);
		break;
	case LOADFILE_ZONE:
		buf = (byte *) Z_Malloc (len+1);
		break;
	case LOADFILE_CACHE:
		buf = (byte *) Cache_Alloc (loadcache, len+1, base);
		break;
	case LOADFILE_STACK:
		if (len < loadsize)
			buf = loadbuf;
		else
			buf = (byte *) Hunk_TempAlloc (len+1);
		break;
	case LOADFILE_MALLOC:
		buf = (byte *) malloc (len+1);
		break;
	default:
		Sys_Error ("COM_LoadFile: bad usehunk");
	}

	if (!buf)
		Sys_Error ("COM_LoadFile: not enough space for %s", path);

	((byte *)buf)[len] = 0;

	nread = Sys_FileRead (h, buf, len);
	COM_CloseFile (h);
	if (nread != len)
		Sys_Error ("COM_LoadFile: Error reading %s", path);

	return buf;
}

byte *COM_LoadHunkFile (const char *path, unsigned int *path_id)
{
	return COM_LoadFile (path, LOADFILE_HUNK, path_id);
}

byte *COM_LoadZoneFile (const char *path, unsigned int *path_id)
{
	return COM_LoadFile (path, LOADFILE_ZONE, path_id);
}

byte *COM_LoadTempFile (const char *path, unsigned int *path_id)
{
	return COM_LoadFile (path, LOADFILE_TEMPHUNK, path_id);
}

void COM_LoadCacheFile (const char *path, struct cache_user_s *cu, unsigned int *path_id)
{
	loadcache = cu;
	COM_LoadFile (path, LOADFILE_CACHE, path_id);
}

// uses temp hunk if larger than bufsize
byte *COM_LoadStackFile (const char *path, void *buffer, int bufsize, unsigned int *path_id)
{
	byte	*buf;

	loadbuf = (byte *)buffer;
	loadsize = bufsize;
	buf = COM_LoadFile (path, LOADFILE_STACK, path_id);

	return buf;
}

// returns malloc'd memory
byte *COM_LoadMallocFile (const char *path, unsigned int *path_id)
{
	return COM_LoadFile (path, LOADFILE_MALLOC, path_id);
}

byte *COM_LoadMallocFile_TextMode_OSPath (const char *path, long *len_out)
{
	FILE	*f;
	byte	*data;
	long	len, actuallen;

	// ericw -- this is used by Host_Loadgame_f. Translate CRLF to LF on load games,
	// othewise multiline messages have a garbage character at the end of each line.
	// TODO: could handle in a way that allows loading CRLF savegames on mac/linux
	// without the junk characters appearing.
	f = fopen (path, "rt");
	if (f == NULL)
		return NULL;

	len = COM_filelength (f);
	if (len < 0)
	{
		fclose (f);
		return NULL;
	}

	data = (byte *) malloc (len + 1);
	if (data == NULL)
	{
		fclose (f);
		return NULL;
	}

	// (actuallen < len) if CRLF to LF translation was performed
	actuallen = fread (data, 1, len, f);
	if (ferror(f))
	{
		fclose (f);
		free (data);
		return NULL;
	}
	data[actuallen] = '\0';

	if (len_out != NULL)
		*len_out = actuallen;
	fclose (f);
	return data;
}

const char *COM_ParseIntNewline(const char *buffer, int *value)
{
	int consumed = 0;
	sscanf (buffer, "%i\n%n", value, &consumed);
	return buffer + consumed;
}

const char *COM_ParseFloatNewline(const char *buffer, float *value)
{
	int consumed = 0;
	sscanf (buffer, "%f\n%n", value, &consumed);
	return buffer + consumed;
}

const char *COM_ParseStringNewline(const char *buffer)
{
	int consumed = 0;
	com_token[0] = '\0';
	sscanf (buffer, "%1023s\n%n", com_token, &consumed);
	return buffer + consumed;
}

/*
COM_LoadPackFile -- johnfitz -- modified based on topaz's tutorial

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.

PAK files are Quake's archive format (similar to ZIP but simpler).
Structure: header + file data + directory at end.
Contains a directory of filenames with offsets into the archive.
Allows bundling thousands of game files into a single .pak file.

Verifies CRC to detect modified/pirated data files.
JavaScript equivalent: loading a ZIP archive or tarball.
*/
static pack_t *COM_LoadPackFile (const char *packfile)
{
	dpackheader_t	header;
	int		i;
	packfile_t	*newfiles;
	int		numpackfiles;
	pack_t		*pack;
	int		packhandle;
	dpackfile_t	info[MAX_FILES_IN_PACK];
	unsigned short	crc;

	if (Sys_FileOpenRead (packfile, &packhandle) == -1)
		return NULL;

	if (Sys_FileRead(packhandle, &header, sizeof(header)) != (int) sizeof(header) ||
	    header.id[0] != 'P' || header.id[1] != 'A' || header.id[2] != 'C' || header.id[3] != 'K')
		Sys_Error ("%s is not a packfile", packfile);

	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (header.dirlen < 0 || header.dirofs < 0)
	{
		Sys_Error ("Invalid packfile %s (dirlen: %i, dirofs: %i)",
					packfile, header.dirlen, header.dirofs);
	}
	if (!numpackfiles)
	{
		Sys_Printf ("WARNING: %s has no files, ignored\n", packfile);
		Sys_FileClose (packhandle);
		return NULL;
	}
	if (numpackfiles > MAX_FILES_IN_PACK)
		Sys_Error ("%s has %i files", packfile, numpackfiles);

	if (numpackfiles != PAK0_COUNT)
		com_modified = true;	// not the original file

	newfiles = (packfile_t *) Z_Malloc(numpackfiles * sizeof(packfile_t));

	Sys_FileSeek (packhandle, header.dirofs);
	if (Sys_FileRead(packhandle, info, header.dirlen) != header.dirlen)
		Sys_Error ("Error reading %s", packfile);

	// crc the directory to check for modifications
	CRC_Init (&crc);
	for (i = 0; i < header.dirlen; i++)
		CRC_ProcessByte (&crc, ((byte *)info)[i]);
	if (crc != PAK0_CRC_V106 && crc != PAK0_CRC_V101 && crc != PAK0_CRC_V100)
		com_modified = true;

	// parse the directory
	for (i = 0; i < numpackfiles; i++)
	{
		q_strlcpy (newfiles[i].name, info[i].name, sizeof(newfiles[i].name));
		newfiles[i].filepos = LittleLong(info[i].filepos);
		newfiles[i].filelen = LittleLong(info[i].filelen);
	}

	pack = (pack_t *) Z_Malloc (sizeof (pack_t));
	q_strlcpy (pack->filename, packfile, sizeof(pack->filename));
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;

	//Sys_Printf ("Added packfile %s (%i files)\n", packfile, numpackfiles);
	return pack;
}

/*
COM_AddGameDirectory -- johnfitz -- modified based on topaz's tutorial
*/
static void COM_AddGameDirectory (const char *base, const char *dir)
{
	int i;
	unsigned int path_id;
	searchpath_t *search;
	pack_t *pak, *qspak;
	char pakfile[MAX_OSPATH];
	qboolean been_here = false;

	q_strlcpy (com_gamedir, va("%s/%s", base, dir), sizeof(com_gamedir));

	// assign a path_id to this game directory
	if (com_searchpaths)
		path_id = com_searchpaths->path_id << 1;
	else	path_id = 1U;

_add_path:
	// add the directory to the search path
	search = (searchpath_t *) Z_Malloc(sizeof(searchpath_t));
	search->path_id = path_id;
	q_strlcpy (search->filename, com_gamedir, sizeof(search->filename));
	search->next = com_searchpaths;
	com_searchpaths = search;

	// add any pak files in the format pak0.pak pak1.pak, ...
	for (i = 0; ; i++)
	{
		q_snprintf (pakfile, sizeof(pakfile), "%s/pak%i.pak", com_gamedir, i);
		pak = COM_LoadPackFile (pakfile);
		if (i != 0 || path_id != 1 || fitzmode)
			qspak = NULL;
		else {
			qboolean old = com_modified;
			if (been_here) base = host_parms->userdir;
			q_snprintf (pakfile, sizeof(pakfile), "%s/quakespasm.pak", base);
			qspak = COM_LoadPackFile (pakfile);
			com_modified = old;
		}
		if (pak) {
			search = (searchpath_t *) Z_Malloc(sizeof(searchpath_t));
			search->path_id = path_id;
			search->pack = pak;
			search->next = com_searchpaths;
			com_searchpaths = search;
		}
		if (qspak) {
			search = (searchpath_t *) Z_Malloc(sizeof(searchpath_t));
			search->path_id = path_id;
			search->pack = qspak;
			search->next = com_searchpaths;
			com_searchpaths = search;
		}
		if (!pak) break;
	}

	if (!been_here && host_parms->userdir != host_parms->basedir)
	{
		been_here = true;
		q_strlcpy(com_gamedir, va("%s/%s", host_parms->userdir, dir), sizeof(com_gamedir));
		Sys_mkdir(com_gamedir);
		goto _add_path;
	}
}

//==============================================================================
//johnfitz -- dynamic gamedir stuff -- modified by QuakeSpasm team.
//==============================================================================
static void COM_Game_f (void)
{
	if (Cmd_Argc() > 1)
	{
		const char *p = Cmd_Argv(1);
		const char *p2 = Cmd_Argv(2);
		searchpath_t *search;

		if (!registered.value) //disable shareware quake
		{
			Con_Printf("You must have the registered version to use modified games\n");
			return;
		}

		if (!*p || !strcmp(p, ".") || strstr(p, "..") || strstr(p, "/") || strstr(p, "\\") || strstr(p, ":"))
		{
			Con_Printf ("gamedir should be a single directory name, not a path\n");
			return;
		}

		if (*p2)
		{
			if (strcmp(p2,"-hipnotic") && strcmp(p2,"-rogue") && strcmp(p2,"-quoth")) {
				Con_Printf ("invalid mission pack argument to \"game\"\n");
				return;
			}
			if (!q_strcasecmp(p, GAMENAME)) {
				Con_Printf ("no mission pack arguments to %s game\n", GAMENAME);
				return;
			}
		}

		if (Sys_FileType(va("%s/%s", com_basedir, p)) != FS_ENT_DIRECTORY)
		{
			if (host_parms->userdir == host_parms->basedir || (Sys_FileType(va("%s/%s", host_parms->userdir, p)) != FS_ENT_DIRECTORY))
			{
				Con_Printf ("No such game directory \"%s\"\n", p);
				return;
			}
		}

		if (!q_strcasecmp(p, COM_SkipPath(com_gamedir))) //no change
		{
			if (com_searchpaths->path_id > 1) { //current game not id1
				if (*p2 && com_searchpaths->path_id == 2) {
					// rely on QuakeSpasm extension treating '-game missionpack'
					// as '-missionpack', otherwise would be a mess
					if (!q_strcasecmp(p, &p2[1]))
						goto _same;
					Con_Printf("reloading game \"%s\" with \"%s\" support\n", p, &p2[1]);
				}
				else if (!*p2 && com_searchpaths->path_id > 2)
					Con_Printf("reloading game \"%s\" without mission pack support\n", p);
				else goto _same;
			}
			else { _same:
				Con_Printf("\"game\" is already \"%s\"\n", COM_SkipPath(com_gamedir));
				return;
			}
		}

		com_modified = true;

		//Kill the server
		CL_Disconnect ();
		Host_ShutdownServer(true);

		//Write config file
		Host_WriteConfiguration ();

		//Kill the extra game if it is loaded
		while (com_searchpaths != com_base_searchpaths)
		{
			if (com_searchpaths->pack)
			{
				Sys_FileClose (com_searchpaths->pack->handle);
				Z_Free (com_searchpaths->pack->files);
				Z_Free (com_searchpaths->pack);
			}
			search = com_searchpaths->next;
			Z_Free (com_searchpaths);
			com_searchpaths = search;
		}
		hipnotic = false;
		rogue = false;
		standard_quake = true;

		if (q_strcasecmp(p, GAMENAME)) //game is not id1
		{
			if (*p2) {
				COM_AddGameDirectory (com_basedir, &p2[1]);
				standard_quake = false;
				if (!strcmp(p2,"-hipnotic") || !strcmp(p2,"-quoth"))
					hipnotic = true;
				else if (!strcmp(p2,"-rogue"))
					rogue = true;
				if (q_strcasecmp(p, &p2[1])) //don't load twice
					COM_AddGameDirectory (com_basedir, p);
			}
			else {
				COM_AddGameDirectory (com_basedir, p);
				// QuakeSpasm extension: treat '-game missionpack' as '-missionpack'
				if (!q_strcasecmp(p,"hipnotic") || !q_strcasecmp(p,"quoth")) {
					hipnotic = true;
					standard_quake = false;
				}
				else if (!q_strcasecmp(p,"rogue")) {
					rogue = true;
					standard_quake = false;
				}
			}
		}
		else // just update com_gamedir
		{
			q_snprintf (com_gamedir, sizeof(com_gamedir), "%s/%s",
					(host_parms->userdir != host_parms->basedir)?
						   host_parms->userdir : com_basedir,
					GAMENAME);
		}

		//clear out and reload appropriate data
		Cache_Flush ();
		Mod_ResetAll();
		Sky_ClearAll();
		if (!isDedicated)
		{
			TexMgr_NewGame ();
			Draw_NewGame ();
			R_NewGame ();
		}
		ExtraMaps_NewGame ();
		Host_Resetdemos ();
		DemoList_Rebuild ();

		Con_Printf("\"game\" changed to \"%s\"\n", COM_SkipPath(com_gamedir));

		VID_Lock ();
		Cbuf_AddText ("exec quake.rc\n");
		Cbuf_AddText ("vid_unlock\n");
	}
	else //Diplay the current gamedir
		Con_Printf("\"game\" is \"%s\"\n", COM_SkipPath(com_gamedir));
}

/*
COM_InitFilesystem

Initializes Quake's virtual filesystem.

Sets up search paths in priority order:
1. Current game directory (mod)
2. Mission pack directory (if specified: rogue, hipnotic, quoth)
3. Base game directory (id1)

Each directory can contain PAK files (pak0.pak, pak1.pak, etc.) and loose files.
Loose files override PAK files. Mods override base game.

This creates a unified view where you request "maps/e1m1.bsp" and it finds
the file regardless of whether it's in a PAK or loose on disk.

JavaScript equivalent: setting up a virtual filesystem or module resolution paths.
*/
void COM_InitFilesystem (void) //johnfitz -- modified based on topaz's tutorial
{
	int i, j;

	Cvar_RegisterVariable (&registered);
	Cvar_RegisterVariable (&cmdline);
	Cmd_AddCommand ("path", COM_Path_f);
	Cmd_AddCommand ("game", COM_Game_f); //johnfitz

	i = COM_CheckParm ("-basedir");
	if (i && i < com_argc-1)
		q_strlcpy (com_basedir, com_argv[i + 1], sizeof(com_basedir));
	else
		q_strlcpy (com_basedir, host_parms->basedir, sizeof(com_basedir));

	j = strlen (com_basedir);
	if (j < 1) Sys_Error("Bad argument to -basedir");
	if ((com_basedir[j-1] == '\\') || (com_basedir[j-1] == '/'))
		com_basedir[j-1] = 0;

	// start up with GAMENAME by default (id1)
	COM_AddGameDirectory (com_basedir, GAMENAME);

	/* this is the end of our base searchpath:
	 * any set gamedirs, such as those from -game command line
	 * arguments or by the 'game' console command will be freed
	 * up to here upon a new game command. */
	com_base_searchpaths = com_searchpaths;

	// add mission pack requests (only one should be specified)
	if (COM_CheckParm ("-rogue"))
		COM_AddGameDirectory (com_basedir, "rogue");
	if (COM_CheckParm ("-hipnotic"))
		COM_AddGameDirectory (com_basedir, "hipnotic");
	if (COM_CheckParm ("-quoth"))
		COM_AddGameDirectory (com_basedir, "quoth");

	i = COM_CheckParm ("-game");
	if (i && i < com_argc-1)
	{
		const char *p = com_argv[i + 1];
		if (!*p || !strcmp(p, ".") || strstr(p, "..") || strstr(p, "/") || strstr(p, "\\") || strstr(p, ":"))
			Sys_Error ("gamedir should be a single directory name, not a path\n");
		com_modified = true;
		// don't load mission packs twice
		if (COM_CheckParm ("-rogue") && !q_strcasecmp(p, "rogue")) p = NULL;
		if (p && COM_CheckParm ("-hipnotic") && !q_strcasecmp(p, "hipnotic")) p = NULL;
		if (p && COM_CheckParm ("-quoth") && !q_strcasecmp(p, "quoth")) p = NULL;
		if (p != NULL) {
			COM_AddGameDirectory (com_basedir, p);
			// QuakeSpasm extension: treat '-game missionpack' as '-missionpack'
			if (!q_strcasecmp(p,"rogue")) {
				rogue = true;
				standard_quake = false;
			}
			if (!q_strcasecmp(p,"hipnotic") || !q_strcasecmp(p,"quoth")) {
				hipnotic = true;
				standard_quake = false;
			}
		}
	}

	COM_CheckRegistered ();
}


/* The following FS_*() stdio replacements are necessary if one is
 * to perform non-sequential reads on files reopened on pak files
 * because we need the bookkeeping about file start/end positions.
 * Allocating and filling in the fshandle_t structure is the users'
 * responsibility when the file is initially opened. */

size_t FS_fread(void *ptr, size_t size, size_t nmemb, fshandle_t *fh)
{
	long byte_size;
	long bytes_read;
	size_t nmemb_read;

	if (!fh) {
		errno = EBADF;
		return 0;
	}
	if (!ptr) {
		errno = EFAULT;
		return 0;
	}
	if (!size || !nmemb) {	/* no error, just zero bytes wanted */
		errno = 0;
		return 0;
	}

	byte_size = nmemb * size;
	if (byte_size > fh->length - fh->pos)	/* just read to end */
		byte_size = fh->length - fh->pos;
	bytes_read = fread(ptr, 1, byte_size, fh->file);
	fh->pos += bytes_read;

	/* fread() must return the number of elements read,
	 * not the total number of bytes. */
	nmemb_read = bytes_read / size;
	/* even if the last member is only read partially
	 * it is counted as a whole in the return value. */
	if (bytes_read % size)
		nmemb_read++;

	return nmemb_read;
}

int FS_fseek(fshandle_t *fh, long offset, int whence)
{
/* I don't care about 64 bit off_t or fseeko() here.
 * the quake/hexen2 file system is 32 bits, anyway. */
	int ret;

	if (!fh) {
		errno = EBADF;
		return -1;
	}

	/* the relative file position shouldn't be smaller
	 * than zero or bigger than the filesize. */
	switch (whence)
	{
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += fh->pos;
		break;
	case SEEK_END:
		offset = fh->length + offset;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (offset < 0) {
		errno = EINVAL;
		return -1;
	}

	if (offset > fh->length)	/* just seek to end */
		offset = fh->length;

	ret = fseek(fh->file, fh->start + offset, SEEK_SET);
	if (ret < 0)
		return ret;

	fh->pos = offset;
	return 0;
}

int FS_fclose(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fclose(fh->file);
}

long FS_ftell(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fh->pos;
}

void FS_rewind(fshandle_t *fh)
{
	if (!fh) return;
	clearerr(fh->file);
	fseek(fh->file, fh->start, SEEK_SET);
	fh->pos = 0;
}

int FS_feof(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	if (fh->pos >= fh->length)
		return -1;
	return 0;
}

int FS_ferror(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return ferror(fh->file);
}

int FS_fgetc(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return EOF;
	}
	if (fh->pos >= fh->length)
		return EOF;
	fh->pos += 1;
	return fgetc(fh->file);
}

char *FS_fgets(char *s, int size, fshandle_t *fh)
{
	char *ret;

	if (FS_feof(fh))
		return NULL;

	if (size > (fh->length - fh->pos) + 1)
		size = (fh->length - fh->pos) + 1;

	ret = fgets(s, size, fh->file);
	fh->pos = ftell(fh->file) - fh->start;

	return ret;
}

long FS_filelength (fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fh->length;
}

/*
								LOCALIZATION
*/
typedef struct
{
	char *key;
	char *value;
} locentry_t;

typedef struct
{
	int			numentries;
	int			maxnumentries;
	int			numindices;
	unsigned	*indices;
	locentry_t	*entries;
	char		*text;
} localization_t;

static localization_t localization;

/*
COM_HashString

Computes the FNV-1a hash of string str

FNV-1a is a fast, non-cryptographic hash function.
Used for hash table lookups in the localization system.
Converts a string into a number for fast dictionary-style lookup.
JavaScript equivalent: Map() uses similar hashing internally.
*/
unsigned COM_HashString (const char *str)
{
	unsigned hash = 0x811c9dc5u;
	while (*str)
	{
		hash ^= *str++;
		hash *= 0x01000193u;
	}
	return hash;
}

static size_t mz_zip_file_read_func(void *opaque, mz_uint64 ofs, void *buf, size_t n)
{
	if (SDL_RWseek((SDL_RWops*)opaque, (Sint64)ofs, RW_SEEK_SET) < 0)
		return 0;
	#ifdef USE_SDL2
	return SDL_RWread((SDL_RWops*)opaque, buf, 1, n);
	#else
	else {
		int r = SDL_RWread((SDL_RWops*)opaque, buf, 1, n);
		return (r < 0)? 0 : r;
	}
	#endif
}

#ifndef USE_SDL2 /* no SDL_RWsize in SDL-1.2 */
static Sint32 SDLCALL SDL_RWsize(SDL_RWops *rw) {
	Sint32 pos, size;
	if ((pos=SDL_RWtell(rw))<0) return -1;
	size = SDL_RWseek(rw, 0, RW_SEEK_END);
	SDL_RWseek(rw, pos, RW_SEEK_SET);
	return size;
}
#endif

/*
LOC_LoadFile

Loads a localization file (translation strings).
Format: key=value pairs, supports escape sequences.
Builds a hash table for fast lookup of translated strings.

Can load from:
1. Plain text file
2. QuakeEX.kpf archive (ZIP-based) using miniz

Parses the file, builds hash table with 50% load factor for efficiency.
JavaScript equivalent: loading a JSON language file into a Map.
*/
void LOC_LoadFile (const char *file)
{
	char path[1024];
	int i,lineno;
	char *cursor;

	SDL_RWops *rw = NULL;
	Sint64 sz;
	mz_zip_archive archive;
	size_t size = 0;

	// clear existing data
	if (localization.text)
	{
		free(localization.text);
		localization.text = NULL;
	}
	localization.numentries = 0;
	localization.numindices = 0;

	if (!file || !*file)
		return;

	Con_Printf("\nLanguage initialization\n");

	memset(&archive, 0, sizeof(archive));
	q_snprintf(path, sizeof(path), "%s/%s", com_basedir, file);
	rw = SDL_RWFromFile(path, "rb");
	#if defined(DO_USERDIRS)
	if (!rw) {
		q_snprintf(path, sizeof(path), "%s/%s", host_parms->userdir, file);
		rw = SDL_RWFromFile(path, "rb");
	}
	#endif
	if (!rw)
	{
		q_snprintf(path, sizeof(path), "%s/QuakeEX.kpf", com_basedir);
		rw = SDL_RWFromFile(path, "rb");
		#if defined(DO_USERDIRS)
		if (!rw) {
			q_snprintf(path, sizeof(path), "%s/QuakeEX.kpf", host_parms->userdir);
			rw = SDL_RWFromFile(path, "rb");
		}
		#endif
		if (!rw) goto fail;
		sz = SDL_RWsize(rw);
		if (sz <= 0) goto fail;
		archive.m_pRead = mz_zip_file_read_func;
		archive.m_pIO_opaque = rw;
		if (!mz_zip_reader_init(&archive, sz, 0)) goto fail;
		localization.text = (char *) mz_zip_reader_extract_file_to_heap(&archive, file, &size, 0);
		if (!localization.text) goto fail;
		mz_zip_reader_end(&archive);
		SDL_RWclose(rw);
		localization.text = (char *) realloc(localization.text, size+1);
		localization.text[size] = 0;
	}
	else
	{
		sz = SDL_RWsize(rw);
		if (sz <= 0) goto fail;
		localization.text = (char *) calloc(1, sz+1);
		if (!localization.text)
		{
fail:			mz_zip_reader_end(&archive);
			if (rw) SDL_RWclose(rw);
			Con_Printf("Couldn't load '%s'\nfrom '%s'\n", file, com_basedir);
			return;
		}
		SDL_RWread(rw, localization.text, 1, sz);
		SDL_RWclose(rw);
	}

	cursor = localization.text;

	// skip BOM
	if ((unsigned char)(cursor[0]) == 0xEF && (unsigned char)(cursor[1]) == 0xBB && (unsigned char)(cursor[2]) == 0xBF)
		cursor += 3;

	lineno = 0;
	while (*cursor)
	{
		char *line, *equals;

		lineno++;

		// skip leading whitespace
		while (q_isblank(*cursor))
			++cursor;

		line = cursor;
		equals = NULL;
		// find line end and first equals sign, if any
		while (*cursor && *cursor != '\n')
		{
			if (*cursor == '=' && !equals)
				equals = cursor;
			cursor++;
		}

		if (line[0] == '/')
		{
			if (line[1] != '/')
				Con_DPrintf("LOC_LoadFile: malformed comment on line %d\n", lineno);
		}
		else if (equals)
		{
			char *key_end = equals;
			qboolean leading_quote;
			qboolean trailing_quote;
			locentry_t *entry;
			char *value_src;
			char *value_dst;
			char *value;

			// trim whitespace before equals sign
			while (key_end != line && q_isspace(key_end[-1]))
				key_end--;
			*key_end = 0;

			value = equals + 1;
			// skip whitespace after equals sign
			while (value != cursor && q_isspace(*value))
				value++;

			leading_quote = (*value == '\"');
			trailing_quote = false;
			value += leading_quote;

			// transform escape sequences in-place
			value_src = value;
			value_dst = value;
			while (value_src != cursor)
			{
				if (*value_src == '\\' && value_src + 1 != cursor)
				{
					char c = value_src[1];
					value_src += 2;
					switch (c)
					{
						case 'n': *value_dst++ = '\n'; break;
						case 't': *value_dst++ = '\t'; break;
						case 'v': *value_dst++ = '\v'; break;
						case 'b': *value_dst++ = '\b'; break;
						case 'f': *value_dst++ = '\f'; break;

						case '"':
						case '\'':
							*value_dst++ = c;
							break;

						default:
							Con_Printf("LOC_LoadFile: unrecognized escape sequence \\%c on line %d\n", c, lineno);
							*value_dst++ = c;
							break;
					}
					continue;
				}

				if (*value_src == '\"')
				{
					trailing_quote = true;
					*value_dst = 0;
					break;
				}

				*value_dst++ = *value_src++;
			}

			// if not a quoted string, trim trailing whitespace
			if (!trailing_quote)
			{
				while (value_dst != value && q_isblank(value_dst[-1]))
				{
					*value_dst = 0;
					value_dst--;
				}
			}

			if (localization.numentries == localization.maxnumentries)
			{
				// grow by 50%
				localization.maxnumentries += localization.maxnumentries >> 1;
				localization.maxnumentries = q_max(localization.maxnumentries, 32);
				localization.entries = (locentry_t*) realloc(localization.entries, sizeof(*localization.entries) * localization.maxnumentries);
			}

			entry = &localization.entries[localization.numentries++];
			entry->key = line;
			entry->value = value;
		}

		if (*cursor)
			*cursor++ = 0; // terminate line and advance to next
	}

	// hash all entries

	localization.numindices = localization.numentries * 2; // 50% load factor
	if (localization.numindices == 0)
	{
		Con_Printf("No localized strings in file '%s'\n", file);
		return;
	}

	localization.indices = (unsigned*) realloc(localization.indices, localization.numindices * sizeof(*localization.indices));
	memset(localization.indices, 0, localization.numindices * sizeof(*localization.indices));

	for (i = 0; i < localization.numentries; i++)
	{
		locentry_t *entry = &localization.entries[i];
		unsigned pos = COM_HashString(entry->key) % localization.numindices, end = pos;

		for (;;)
		{
			if (!localization.indices[pos])
			{
				localization.indices[pos] = i + 1;
				break;
			}

			++pos;
			if (pos == localization.numindices)
				pos = 0;

			if (pos == end)
				Sys_Error("LOC_LoadFile failed");
		}
	}

	Con_Printf("Loaded %d strings from '%s'\n", localization.numentries, file);
}

/*
LOC_Init
*/
void LOC_Init(void)
{
	LOC_LoadFile("localization/loc_english.txt");
}

/*
LOC_Shutdown
*/
void LOC_Shutdown(void)
{
	free(localization.indices);
	free(localization.entries);
	free(localization.text);
}

/*
LOC_GetRawString

Returns localized string if available, or NULL otherwise
*/
const char* LOC_GetRawString (const char *key)
{
	unsigned pos, end;

	if (!localization.numindices || !key || !*key || *key != '$')
		return NULL;
	key++;

	pos = COM_HashString(key) % localization.numindices;
	end = pos;

	do
	{
		unsigned idx = localization.indices[pos];
		locentry_t *entry;
		if (!idx)
			return NULL;

		entry = &localization.entries[idx - 1];
		if (!Q_strcmp(entry->key, key))
			return entry->value;

		++pos;
		if (pos == localization.numindices)
			pos = 0;
	} while (pos != end);

	return NULL;
}

/*
LOC_GetString

Returns localized string if available, or input string otherwise

If key starts with '$', looks it up in the localization table.
Otherwise returns the key itself as a fallback.
This allows graceful handling of missing translations.
JavaScript: like i18n.t(key) with automatic fallback.
*/
const char* LOC_GetString (const char *key)
{
	const char* value = LOC_GetRawString(key);
	return value ? value : key;
}

/*
LOC_ParseArg

Returns argument index (>= 0) and advances the string if it starts with a placeholder ({} or {N}),
otherwise returns a negative value and leaves the pointer unchanged
*/
static int LOC_ParseArg (const char **pstr)
{
	int arg;
	const char *str = *pstr;

	// opening brace
	if (*str != '{')
		return -1;
	++str;

	// optional index, defaulting to 0
	arg = 0;
	while (q_isdigit(*str))
		arg = arg * 10 + *str++ - '0';

	// closing brace
	if (*str != '}')
		return -1;
	*pstr = ++str;

	return arg;
}

/*
LOC_HasPlaceholders
*/
qboolean LOC_HasPlaceholders (const char *str)
{
	if (!localization.numindices)
		return false;
	while (*str)
	{
		if (LOC_ParseArg(&str) >= 0)
			return true;
		str++;
	}
	return false;
}

/*
LOC_Format

Replaces placeholders (of the form {} or {N}) with the corresponding arguments

Returns number of written chars, excluding the NUL terminator
If len > 0, output is always NUL-terminated

Like sprintf but with {} placeholders instead of % format specifiers.
{} uses auto-incrementing index, {0} {1} etc. for specific arguments.
JavaScript equivalent: template literals `Player {0} scored {1} points`
but with runtime argument substitution via callback function.
*/
size_t LOC_Format (const char *format, const char* (*getarg_fn) (int idx, void* userdata), void* userdata, char* out, size_t len)
{
	size_t written = 0;
	int numargs = 0;

	if (!len)
	{
		Con_DPrintf("LOC_Format: no output space\n");
		return 0;
	}
	--len; // reserve space for the terminator

	while (*format && written < len)
	{
		const char* insert;
		size_t space_left;
		size_t insert_len;
		int argindex = LOC_ParseArg(&format);

		if (argindex < 0)
		{
			out[written++] = *format++;
			continue;
		}

		insert = getarg_fn(argindex, userdata);
		space_left = len - written;
		insert_len = Q_strlen(insert);

		if (insert_len > space_left)
		{
			Con_DPrintf("LOC_Format: overflow at argument #%d\n", numargs);
			insert_len = space_left;
		}

		Q_memcpy(out + written, insert, insert_len);
		written += insert_len;
	}

	if (*format)
		Con_DPrintf("LOC_Format: overflow\n");

	out[written] = 0;

	return written;
}
