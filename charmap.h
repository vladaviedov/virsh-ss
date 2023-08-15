#pragma once

#define KEY_GRAVE "KEY_GRAVE"
#define KEY_1 "KEY_1"
#define KEY_2 "KEY_2"
#define KEY_3 "KEY_3"
#define KEY_4 "KEY_4"
#define KEY_5 "KEY_5"
#define KEY_6 "KEY_6"
#define KEY_7 "KEY_7"
#define KEY_8 "KEY_8"
#define KEY_9 "KEY_9"
#define KEY_0 "KEY_0"
#define KEY_MINUS "KEY_MINUS"
#define KEY_EQUAL "KEY_EQUAL"
#define KEY_LEFTBRACE "KEY_LEFTBRACE"
#define KEY_RIGHTBRACE "KEY_RIGHTBRACE"
#define KEY_BACKSLASH "KEY_BACKSLASH"
#define KEY_SEMICOLON "KEY_SEMICOLON"
#define KEY_APOSTROPHE "KEY_APOSTROPHE"
#define KEY_COMMA "KEY_COMMA"
#define KEY_DOT "KEY_DOT"

typedef struct {
	const char unshifted;
	const char shifted;
	const char *formatted;
} kb_key;

#define MISC_KEY_COUNT (sizeof(misc_keys) / sizeof(misc_keys[0]))
static const kb_key misc_keys[] = {
	{ '`', '~', KEY_GRAVE },
	{ '1', '!', KEY_1 },
	{ '2', '@', KEY_2 },
	{ '3', '#', KEY_3 },
	{ '4', '$', KEY_4 },
	{ '5', '%', KEY_5 },
	{ '6', '^', KEY_6 },
	{ '7', '&', KEY_7 },
	{ '8', '*', KEY_8 },
	{ '9', '(', KEY_9 },
	{ '0', ')', KEY_0 },
	{ '-', '_', KEY_MINUS },
	{ '=', '+', KEY_EQUAL },
	{ '[', '{', KEY_LEFTBRACE },
	{ ']', '}', KEY_RIGHTBRACE },
	{ '\\', '|', KEY_BACKSLASH },
	{ ';', ':', KEY_SEMICOLON },
	{ '\'', '"', KEY_APOSTROPHE },
	{ ',', '<', KEY_COMMA },
	{ '.', '>', KEY_DOT }
};
