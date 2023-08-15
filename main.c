#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "charmap.h"

#define VIRSH_SS "virsh-ss"
#define VIRSH_SS_VERSION "0.1"

#define VERSION_STR "--version"
#define HELP_STR "--help"

#define VIRSH "virsh"
#define SEND_KEY "send-key"
#define SHIFT_KEY "KEY_LEFTSHIFT"

void print_usage(void);
int verify_key(const char c);
int send_key(const char *domain, const char c);
int is_shifted(const char c);
void format_key(const char c, const int shifted, char *buffer, const uint32_t buffer_size);
int run_virsh(const char **args);

// virsh send-key DOMAIN KEYS
// virsh-ss DOMAIN STRING
int main(int argc, char **argv) {
	// Check for special commands
	char *last_arg = argv[argc - 1];
	if (strcmp(last_arg, VERSION_STR) == 0) {
		printf("%s v%s\n", VIRSH_SS, VIRSH_SS_VERSION);
		return EXIT_SUCCESS;
	}
	if (strcmp(last_arg, HELP_STR) == 0) {
		print_usage();
		return EXIT_SUCCESS;
	}

	// Check arg count
	if (argc != 3) {
		fprintf(stderr, "%s: invalid arguments\n", VIRSH_SS);
		print_usage();
		return EXIT_FAILURE;
	}

	const char *domain = argv[1];
	const char *input = argv[2];

	// Verify keys
	for (uint32_t i = 0; i < strlen(input); i++) {
		if (!verify_key(input[i])) {
			fprintf(stderr, "%s: unsupported key -- %c", VIRSH_SS, input[i]);
		};
	}

	// Send keys
	for (uint32_t i = 0; i < strlen(input); i++) {
		if (send_key(domain, input[i]) != EXIT_SUCCESS) {
			fprintf(stderr, "%s: failed to send keys\n", VIRSH_SS);
			if (i > 0) {
				fprintf(stderr, "wmacrosarning: %u keys have been sent\n", i);
			}
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

/**
 * @brief Print usage information.
 */
void print_usage(void) {
	printf("usage: %s <domain> <string>\n", VIRSH_SS);
	printf("\tdomain - libvirt domain name\n");
	printf("\tstring - string to send\n");
}

/**
 * @brief Verify that we can send this key.
 *
 * @param[in] c - Character to check.
 * @return Boolean result.
 */
int verify_key(const char c) {
	if (isupper(c) || islower(c)) {
		return 1;
	}

	for (uint32_t i = 0; i < MISC_KEY_COUNT; i++) {
		const kb_key *entry = misc_keys + i;

		if (c == entry->unshifted || c == entry->shifted) {
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Send single key to a libvirt domain.
 *
 * @param[in] domain - Libvirt domain.
 * @param[in] c - Character to send
 * @return Exit code.
 */
int send_key(const char *domain, const char c) {
	int shifted = is_shifted(c);

	// Make key
	char key_name[32];
	format_key(c, shifted, key_name, 32);

	// virsh send-key DOMAIN (KEY_LEFTSHIFT) KEY NULL
	const char *args[5 + shifted];
	args[0] = VIRSH;
	args[1] = SEND_KEY;
	args[2] = domain;

	if (shifted) {
		args[3] = SHIFT_KEY;
		args[4] = key_name;
		args[5] = NULL;
	} else {
		args[3] = key_name;
		args[4] = NULL;
	}

	printf("debug: sending command\n");
	printf("%s\n", args[0]);
	printf("%s\n", args[1]);
	printf("%s\n", args[2]);
	printf("%s\n", args[3]);
	if (shifted) {
		printf("%s\n", args[4]);
	}

	return run_virsh(args);
}

/**
 * @brief Check if key requires shift to type.
 *
 * @param[in] c - Character to check.
 * @return Boolean result.
 */
int is_shifted(const char c) {
	if (isupper(c)) {
		return 1;
	}

	for (uint32_t i = 0; i < MISC_KEY_COUNT; i++) {
		const kb_key *entry = misc_keys + i;

		if (c == entry->shifted) {
			return 1;
		}
		if (c == entry->unshifted) {
			return 0;
		}
	}

	return 0;
}

/**
 * @brief Format key to format understood by virsh.
 *
 * @param[in] c - Character to format.
 * @param[in] shifted - If character requires shift.
 * @param[out] buffer - Buffer to fill.
 * @param[in] buffer_size - Size of buffer to fill.
 */
void format_key(const char c, const int shifted, char *buffer, const uint32_t buffer_size) {
	// Format normal characters
	if (isupper(c) || islower(c)) {
		snprintf(buffer, buffer_size, "KEY_%c", toupper(c));
		return;
	}

	// Format misc characters
	const char *formatted;
	for (uint32_t i = 0; i < MISC_KEY_COUNT; i++) {
		const kb_key *entry = misc_keys + i;

		if ((!shifted && c == entry->unshifted) || (shifted && c == entry->shifted)) {
			formatted = entry->formatted;
			break;
		}
	}

	snprintf(buffer, buffer_size, "%s", formatted);
}

/**
 * @brief Run virsh command to send key.
 *
 * @param[in] args - Program arguments.
 * @return Exit code.
 */
int run_virsh(const char **args) {
	// TODO: exec
	return 0;
}
