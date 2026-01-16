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

#ifndef __CVAR_H__
#define __CVAR_H__

/*
cvar_t variables are used to hold scalar or string variables that can
be changed or displayed at the console or prog code as well as accessed
directly in C code.

it is sufficient to initialize a cvar_t with just the first two fields,
or you can add a ,true flag for variables that you want saved to the
configuration file when the game is quit:

cvar_t	r_draworder = {"r_draworder","1"};
cvar_t	scr_screensize = {"screensize","1",true};

Cvars must be registered before use, or they will have a 0 value instead
of the float interpretation of the string.
Generally, all cvar_t declarations should be registered in the apropriate
init function before any console commands are executed:

Cvar_RegisterVariable (&host_framerate);


C code usually just references a cvar in place:
if ( r_draworder.value )

It could optionally ask for the value to be looked up for a string name:
if (Cvar_VariableValue ("r_draworder"))

Interpreted prog code can access cvars with the cvar(name) or
cvar_set (name, value) internal functions:
teamplay = cvar("teamplay");
cvar_set ("registered", "1");

The user can access cvars from the console in two ways:
r_draworder		prints the current value
r_draworder 0		sets the current value to 0

Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.

*/

#define	CVAR_NONE		0
#define	CVAR_ARCHIVE		(1U << 0)	// if set, causes it to be saved to config
#define	CVAR_NOTIFY		(1U << 1)	// changes will be broadcasted to all players (q1)
#define	CVAR_SERVERINFO		(1U << 2)	// added to serverinfo will be sent to clients (q1/net_dgrm.c and qwsv)
#define	CVAR_USERINFO		(1U << 3)	// added to userinfo, will be sent to server (qwcl)
#define	CVAR_CHANGED		(1U << 4)
#define	CVAR_ROM		(1U << 6)
#define	CVAR_LOCKED		(1U << 8)	// locked temporarily
#define	CVAR_REGISTERED		(1U << 10)	// the var is added to the list of variables
#define	CVAR_CALLBACK		(1U << 16)	// var has a callback


typedef void (*cvarcallback_t) (struct cvar_s *);

/*
 * cvar_t - Console Variable structure
 *
 * This is Quake's system for storing configurable variables that can be:
 * - Changed at runtime via the console (like developer tools in a browser)
 * - Saved to config files (like localStorage in JS)
 * - Modified by game code
 * - Accessed by the scripting language (QuakeC)
 *
 * Think of cvars as a dynamic configuration system similar to a JavaScript object
 * that holds game settings, but with extra features for persistence, networking,
 * and type conversion.
 *
 * Why use structs instead of a JavaScript-style object?
 * In C, we define the exact memory layout and types upfront. This struct is like
 * a blueprint that says "every cvar takes exactly this much memory and has these
 * specific fields in this order." This gives us speed and predictability that
 * JavaScript's dynamic objects can't provide.
 */
typedef struct cvar_s
{
	/*
	 * name - The identifier for this variable (e.g., "sv_maxspeed", "r_draworder")
	 *
	 * This is 'const char*' meaning it points to a read-only string. Once set during
	 * registration, the name never changes. In JavaScript terms, this would be like
	 * Object.freeze() on the key of a Map entry.
	 */
	const char	*name;

	/*
	 * string - The current value as a text string
	 *
	 * Why store values as strings? Because cvars can be set from:
	 * - The console (text input)
	 * - Config files (text files)
	 * - Network messages (serialized text)
	 *
	 * String is the "source of truth." Similar to how form inputs in HTML are
	 * always strings, and you parse them to numbers when needed. This pointer
	 * points to dynamically allocated memory that gets freed/reallocated when
	 * the value changes (see Cvar_SetQuick).
	 */
	const char	*string;

	/*
	 * flags - Bit flags controlling behavior (CVAR_ARCHIVE, CVAR_NOTIFY, CVAR_ROM, etc.)
	 *
	 * In C, we use bit flags to pack multiple boolean settings into a single integer.
	 * Each flag is a power of 2, so they can be combined with bitwise OR (|).
	 *
	 * Example: flags = CVAR_ARCHIVE | CVAR_NOTIFY means "save to config AND broadcast changes"
	 *
	 * In JavaScript, you might use an object like {archive: true, notify: true},
	 * but bit flags are more memory-efficient (1 int vs multiple properties)
	 * and faster to check (one bitwise AND operation vs property lookup).
	 */
	unsigned int	flags;

	/*
	 * value - Cached floating-point conversion of the string value
	 *
	 * For performance, we pre-parse the string to a float when it's set.
	 * This avoids calling atof() (string-to-float conversion) every time
	 * code checks the value.
	 *
	 * Think of this like memoization in JavaScript - we compute it once and
	 * cache it. When code does `if (r_draworder.value)`, it's instant because
	 * the conversion already happened.
	 *
	 * Note: 'float' is a 32-bit floating point number in C, similar to JavaScript's
	 * Number but with lower precision than the 64-bit doubles JS uses.
	 */
	float		value;

	/*
	 * default_string - The original/initial value for the reset command
	 *
	 * Added by johnfitz for QuakeSpasm. When Cvar_RegisterVariable is called,
	 * it saves the initial value here. The 'reset' console command restores
	 * the cvar to this value.
	 *
	 * This is like keeping a backup copy of the initial state, similar to
	 * storing initialState in a React component before allowing modifications.
	 */
	const char	*default_string;

	/*
	 * callback - Function pointer called when the cvar's value changes
	 *
	 * This is C's version of event handlers! When a cvar is modified,
	 * if this is set (and CVAR_CALLBACK flag is set), this function gets called.
	 *
	 * In JavaScript: cvar.addEventListener('change', callback)
	 * In C: cvar->callback = myFunction; cvar->flags |= CVAR_CALLBACK;
	 *
	 * cvarcallback_t is a typedef for: void (*)(struct cvar_s*)
	 * Which means: "a pointer to a function that takes a cvar_s pointer and returns nothing"
	 *
	 * This allows code to react to changes, like updating the screen resolution
	 * when the user changes video settings.
	 */
	cvarcallback_t	callback;

	/*
	 * next - Pointer to the next cvar in the linked list
	 *
	 * This is how we implement a linked list in C - each node points to the next one.
	 * The last cvar in the list has next = NULL.
	 *
	 * In JavaScript, you might build this as:
	 * const list = { value: cvar1, next: { value: cvar2, next: { value: cvar3, next: null }}}
	 *
	 * 'struct cvar_s*' means "pointer to another cvar_s struct". The 's' suffix
	 * is a naming convention (cvar_s for the struct, cvar_t for the typedef).
	 */
	struct cvar_s	*next;
} cvar_t;

void	Cvar_RegisterVariable (cvar_t *variable);
// registers a cvar that already has the name, string, and optionally
// the archive elements set.

void Cvar_SetCallback (cvar_t *var, cvarcallback_t func);
// set a callback function to the var

void	Cvar_Set (const char *var_name, const char *value);
// equivelant to "<name> <variable>" typed at the console

void	Cvar_SetValue (const char *var_name, const float value);
// expands value to a string and calls Cvar_Set

void	Cvar_SetROM (const char *var_name, const char *value);
void	Cvar_SetValueROM (const char *var_name, const float value);
// sets a CVAR_ROM variable from within the engine

void Cvar_SetQuick (cvar_t *var, const char *value);
void Cvar_SetValueQuick (cvar_t *var, const float value);
// these two accept a cvar pointer instead of a var name,
// but are otherwise identical to the "non-Quick" versions.
// the cvar MUST be registered.

float	Cvar_VariableValue (const char *var_name);
// returns 0 if not defined or non numeric

const char *Cvar_VariableString (const char *var_name);
// returns an empty string if not defined

qboolean Cvar_Command (void);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void	Cvar_WriteVariables (FILE *f);
// Writes lines containing "set variable value" for all variables
// with the CVAR_ARCHIVE flag set

cvar_t	*Cvar_FindVar (const char *var_name);
cvar_t	*Cvar_FindVarAfter (const char *prev_name, unsigned int with_flags);

void	Cvar_LockVar (const char *var_name);
void	Cvar_UnlockVar (const char *var_name);
void	Cvar_UnlockAll (void);

void	Cvar_Init (void);

const char	*Cvar_CompleteVariable (const char *partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

#endif	/* __CVAR_H__ */

