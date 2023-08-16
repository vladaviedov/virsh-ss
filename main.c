#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <getopt.h>
#include <termios.h>
#include <readline/readline.h>

#include "charmap.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#define VIRSH_SS "virsh-ss"
#define VIRSH_SS_VERSION "0.3"

#define VERSION_STR "--version"
#define HELP_STR "--help"

#define VIRSH "virsh"
#define SEND_KEY "send-key"
#define SHIFT_KEY "KEY_LEFTSHIFT"

#define DEV_NULL "/dev/null"
#define INPUT_SIZE 1024

static int prompt = 0;
static int secret = 0;
static int newline = 0;

static struct option opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ "prompt", no_argument, &prompt, 'p' },
	{ "secret", no_argument, &secret, 's' },
	{ "newline", no_argument, &newline, 'n' }
};

char *get_input(void);
void print_usage(void);
int verify_key(const char c);
int send_key(char *domain, const char c);
int is_shifted(const char c);
void format_key(const char c, const int shifted, char *buffer, const uint32_t buffer_size);
int run_virsh(char *const *args);

int main(int argc, char **argv) {
	char opt;
	while ((opt = getopt_long(argc, argv, "hvps", opts, NULL)) != -1) {
		switch (opt) {
			case 'h':
				print_usage();
				return EXIT_SUCCESS;
			case 'v':
				printf("%s v%s\n", VIRSH_SS, VIRSH_SS_VERSION);
				return EXIT_SUCCESS;
			case '?':
				fprintf(stderr, "%s: invalid argument", VIRSH_SS);
				return EXIT_FAILURE;
			default:
				break;
		}
	}

	if (prompt) prompt = 1;
	if (secret) secret = 1;

	// Check arg count: 1 if prompt, 2 if not prompt
	if (argc - optind != 2 - prompt) {
		fprintf(stderr, "%s: invalid arguments\n", VIRSH_SS);
		print_usage();
		return EXIT_FAILURE;
	}

	char *domain = argv[optind];
	char *input = prompt ? get_input() : argv[optind + 1];

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
				fprintf(stderr, "warning: %u keys have been sent\n", i);
			}
			return EXIT_FAILURE;
		}
	}

	if (newline) {
		if (send_key(domain, '\n') != EXIT_SUCCESS) {
			fprintf(stderr, "%s: failed to send newline\n", VIRSH_SS);
			fprintf(stderr, "warning: string was sent\n");
			return EXIT_FAILURE;
		}
	}

	if (prompt) {
		free(input);
	}

	return EXIT_SUCCESS;
}

char *get_input(void) {
	struct termios term;
	if (secret) {
		tcgetattr(STDIN_FILENO, &term);
		term.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, 0, &term);
	}

	char *input = readline("input string: ");

	if (secret) {
		term.c_lflag |= ECHO;
		tcsetattr(STDIN_FILENO, 0, &term);
	}

	if (!input) {
		fprintf(stderr, "%s: no input was given\n", VIRSH_SS);
		exit(EXIT_FAILURE);
	}

	return input;
}

/**
 * @brief Print usage information.
 */
void print_usage(void) {
	printf("usage: %s [options] <domain> <string>\n", VIRSH_SS);
	printf("\tdomain - libvirt domain name\n");
	printf("\tstring - string to send (if prompt not set)\n");
	printf("options:\n");
	printf("\t-h, --help - show usage information\n");
	printf("\t-v, --version - show program version\n");
	printf("\t-p, --prompt - ask for string as a prompt\n");
	printf("\t-s, --secret - prompt input is hidden if used\n");
	printf("\t-n, --newline - send newline character at the end\n");
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
int send_key(char *domain, const char c) {
	int shifted = is_shifted(c);

	// Make key
	char key_name[32];
	format_key(c, shifted, key_name, 32);

	// virsh send-key DOMAIN (KEY_LEFTSHIFT) KEY NULL
	char *args[5 + shifted];
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

	if (DEBUG) {
		printf("debug: sending command\n");
		printf("\t%s\n", args[0]);
		printf("\t%s\n", args[1]);
		printf("\t%s\n", args[2]);
		printf("\t%s\n", args[3]);
		if (shifted) {
			printf("\t%s\n", args[4]);
		}
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
int run_virsh(char *const *args) {
	// Fork process
	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "%s: failed to fork process\n", VIRSH_SS);
		return EXIT_FAILURE;
	}

	// Child code
	if (pid == 0) {
		// Redirect output to /dev/null
		int dev_null = open(DEV_NULL, O_WRONLY);
		dup2(dev_null, STDOUT_FILENO);
		close(dev_null);

		execvp(args[0], args);
		fprintf(stderr, "%s: faioptionsled to exec virsh\n", VIRSH_SS);
		exit(EXIT_FAILURE);
	}

	// Parent code
	int result;
	waitpid(pid, &result, 0);
	return WEXITSTATUS(result);
}
