/**
 * @file charmap.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 1.0.1
 * @date 2023-2024
 * @license GPLv3.0
 * @brief Mappings for non-alpha key characters.
 */
#pragma once

typedef struct {
	const char unshifted;
	const char shifted;
	const char *formatted;
} kb_key;

#define MISC_KEY_COUNT (sizeof(misc_keys) / sizeof(misc_keys[0]))
const kb_key misc_keys[] = {
	{ '`', '~', "KEY_GRAVE" },
	{ '1', '!', "KEY_1" },
	{ '2', '@', "KEY_2" },
	{ '3', '#', "KEY_3" },
	{ '4', '$', "KEY_4" },
	{ '5', '%', "KEY_5" },
	{ '6', '^', "KEY_6" },
	{ '7', '&', "KEY_7" },
	{ '8', '*', "KEY_8" },
	{ '9', '(', "KEY_9" },
	{ '0', ')', "KEY_0" },
	{ '-', '_', "KEY_MINUS" },
	{ '=', '+', "KEY_EQUAL" },
	{ '[', '{', "KEY_LEFTBRACE" },
	{ ']', '}', "KEY_RIGHTBRACE" },
	{ '\\', '|', "KEY_BACKSLASH" },
	{ ';', ':', "KEY_SEMICOLON" },
	{ '\'', '"', "KEY_APOSTROPHE" },
	{ ',', '<', "KEY_COMMA" },
	{ '.', '>', "KEY_DOT" },
	{ '/', '?', "KEY_SLASH" },
	{ ' ', '\0', "KEY_SPACE" },
	{ '\n', '\0', "KEY_ENTER" },
};
