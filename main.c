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

void print_usage(void);
int send_key(const char *domain, const char c);
int is_shifted(const char c);

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

	// TODO: send string

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
 * @brief Send single key to a libvirt domain.
 *
 * @param[in] domain - Libvirt domain.
 * @param[in] c - Character to send
 * @return Exit code.
 */
int send_key(const char *domain, const char c) {
	// TODO: implement
	return 0;
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
