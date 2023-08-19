#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <getopt.h>
#include <readline/readline.h>

#include "charmap.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#define VIRSH_SS "virsh-ss"
#define VIRSH_SS_VERSION "0.5"

#define VERSION_STR "--version"
#define HELP_STR "--help"

#define VIRSH "virsh"
#define SEND_KEY "send-key"
#define SHIFT_KEY "KEY_LEFTSHIFT"

#define DEV_NULL "/dev/null"
#define INPUT_SIZE 1024

#define DEFAULT_SPEED 1

static int prompt = 0;
static int secret = 0;
static int newline = 0;
static uint32_t speed = DEFAULT_SPEED;

static struct option opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ "prompt", no_argument, &prompt, 'p' },
	{ "secret", no_argument, &secret, 's' },
	{ "newline", no_argument, &newline, 'n' },
	{ "speed", required_argument, 0, 'l' }
};

char *get_input(void);
void print_usage(void);
int verify_key(const char c);
int send_key(char *domain, const char c);
int send_keys(char *domain, const char *c, uint32_t count);
int is_shifted(const char c);
void format_key(const char c, const int shifted, char *buffer, const uint32_t buffer_size);
int run_virsh(char *const *args);
void exit_handler(int code);

int main(int argc, char **argv) {
	char opt;
	while ((opt = getopt_long(argc, argv, "hvpsl:", opts, NULL)) != -1) {
		switch (opt) {
			case 'h':
				print_usage();
				return EXIT_SUCCESS;
			case 'v':
				printf("%s v%s\n", VIRSH_SS, VIRSH_SS_VERSION);
				return EXIT_SUCCESS;
			case 'l': {
				int32_t speed_input = atoi(optarg);
				if (speed_input > 15 || speed_input < 1) {
					fprintf(stderr, "%s: invalid speed value, must be 1-15\n", VIRSH_SS);
					return EXIT_FAILURE;
				}
				speed = speed_input;
				break;
			}
			case '?':
				fprintf(stderr, "%s: invalid argument\n", VIRSH_SS);
				return EXIT_FAILURE;
			default:
				break;
		}
	}

	// Check arg count: 1 if prompt, 2 if not prompt
	if (argc - optind != 2 - !!prompt) {
		fprintf(stderr, "%s: invalid arguments\n", VIRSH_SS);
		print_usage();
		return EXIT_FAILURE;
	}

	// Install signal handlers
	signal(SIGINT, &exit_handler);
	signal(SIGTERM, &exit_handler);
	signal(SIGQUIT, &exit_handler);

	char *domain = argv[optind];
	char *input = prompt ? get_input() : argv[optind + 1];

	// Verify keys
	for (uint32_t i = 0; i < strlen(input); i++) {
		if (!verify_key(input[i])) {
			fprintf(stderr, "%s: unsupported key -- %c\n", VIRSH_SS, input[i]);
		};
	}

	if (speed > 1) {
		// Fast send if applicable
		uint32_t current = 0;
		while (current < strlen(input)) {
			int is_seq_shifted = is_shifted(input[current]);
			uint32_t search = current + 1;

			// Search until speed reached, end of string or different shifting
			while (search - current < speed &&
				   search < strlen(input) &&
				   is_shifted(input[search]) == is_seq_shifted)
			{
				search++;
			}

			// Characters start at index current, end at difference
			if (send_keys(domain, input + current, search - current) != EXIT_SUCCESS) {
				fprintf(stderr, "%s: failed to send keys\n", VIRSH_SS);
				if (current > 0) {
					fprintf(stderr, "warning: %u keys have been sent\n", current);
				}
				return EXIT_FAILURE;
			}

			// Update start
			current += search;
		}
	} else {
		// Send keys one-by-one
		for (uint32_t i = 0; i < strlen(input); i++) {
			if (send_key(domain, input[i]) != EXIT_SUCCESS) {
				fprintf(stderr, "%s: failed to send keys\n", VIRSH_SS);
				if (i > 0) {
					fprintf(stderr, "warning: %u keys have been sent\n", i);
				}
				return EXIT_FAILURE;
			}
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

/**
 * @brief Read input from the user.
 *
 * @return Input string (must be freed).
 */
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
	printf("\t-l, --speed - max amount of characters sent per virsh command (1-15)\n");
	printf("\t              higher values might cause issues (default: 1)\n");
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
 * @param[in] c - Character to send.
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
 * @brief Send 'count' keys to a libvirt domain.
 *
 * @param[in] domain - Libvirt domain.
 * @param[in] c - Character array to send.
 * @param[in] count - Amount of characters in 'c'
 * @return [TODO:description]
 */
int send_keys(char *domain, const char *c, uint32_t count) {
	int shifted = is_shifted(c[0]);

	// Make keys
	char *key_names[count];
	for (uint32_t i = 0; i < count; i++) {
		key_names[i] = malloc(sizeof(char) * 32);
		format_key(c[i], shifted, key_names[i], 32);
	}

	// virsh send-key DOMAIN (KEY_LEFTSHIFT) [KEYS] NULL
	char *args[4 + shifted + count];
	args[0] = VIRSH;
	args[1] = SEND_KEY;
	args[2] = domain;

	if (shifted) {
		args[3] = SHIFT_KEY;
	}

	for (uint32_t i = 0; i < count; i++) {
		args[3 + shifted + i] = key_names[i];
	}
	args[3 + shifted + count] = NULL;

	if (DEBUG) {
		printf("debug: sending command\n");
		for (uint32_t i = 0; i < 3 + shifted + count; i++) {
			printf("\t%s\n", args[i]);
		}
	}

	// Run
	int result = run_virsh(args);
	
	// Cleanup
	for (uint32_t i = 0; i < count; i++) {
		free(key_names[i]);
	}

	return result;
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

/**
 * @brief Return echo to tty if in secret mode and exit.
 *
 * @param[in] code - Exit code.
 */
void exit_handler(int code) {
	if (secret) {
		struct termios term;
		tcgetattr(STDIN_FILENO, &term);
		term.c_lflag |= ECHO;
		tcsetattr(STDIN_FILENO, 0, &term);
	}

	exit(code);
}
