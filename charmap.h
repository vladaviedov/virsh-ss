#pragma once

typedef struct {
	const char unshifted;
	const char shifted;
} kb_key;

#define MISC_KEY_COUNT (sizeof(misc_keys) / sizeof(misc_keys[0]))
static const kb_key misc_keys[] = {
	{ '`', '~' },
	{ '1', '!' },
	{ '2', '@' },
	{ '3', '#' },
	{ '4', '$' },
	{ '5', '%' },
	{ '6', '^' },
	{ '7', '&' },
	{ '8', '*' },
	{ '9', '(' },
	{ '0', ')' },
	{ '-', '_' },
	{ '=', '+' },
	{ '[', '{' },
	{ ']', '}' },
	{ '\\', '|' },
	{ ';', ':' },
	{ '\'', '"' },
	{ ',', '<' },
	{ '.', '>' }
};
