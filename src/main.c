/**
 * @file main.c
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version 0.7
 * @date 2023-2024
 * @license GPLv3.0
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <c-utils/nanorl.h>

#include "charmap.h"

// Debug flag
#ifndef DEBUG
#define DEBUG 0
#endif

// Default virsh binary
#ifndef VIRSH_BIN
#define VIRSH_BIN "virsh"
#endif

// Name used in print messages
#define VIRSH_SS "virsh-ss"
// Software version
#define VIRSH_SS_VERSION "0.7"
// Shift send-key command
#define SHIFT_CMD "KEY_LEFTSHIFT"

static int prompt = 0;
static int secret = 0;
static int newline = 0;
// Default speed
static uint32_t speed = 1;

static struct option opts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'v' },
	{ "prompt", no_argument, NULL, 'p' },
	{ "secret", no_argument, NULL, 's' },
	{ "newline", no_argument, NULL, 'n' },
	{ "speed", required_argument, NULL, 'l' },
};

static char *get_input(void);
static void print_usage(void);
static void print_version(void);
static int verify_key(char c);
static int send_key(char *domain, char c);
static int send_keys(char *domain, const char *keys, uint32_t count);
static char *get_binary_name(void);
static int is_shifted(char c);
static void format_key(char c, int shifted, char *buffer, uint32_t buffer_size);
static int run_virsh(char **args);
static void exit_handler(int code);
static void dispose(char *buffer);

int main(int argc, char **argv) {
	char opt;
	while ((opt = getopt_long(argc, argv, "hvpsnl:", opts, NULL)) != -1) {
		switch (opt) {
		case 'h':
			print_usage();
			return EXIT_SUCCESS;
		case 'v':
			print_version();
			return EXIT_SUCCESS;
		case 'p':
			prompt = 1;
			break;
		case 's':
			secret = 1;
			break;
		case 'n':
			newline = 1;
			break;
		case 'l': {
			int32_t speed_input = atoi(optarg);
			if (speed_input > 15 || speed_input < 1) {
				fprintf(stderr, "%s: invalid speed value, must be 1-15\n",
					VIRSH_SS);
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
	uint32_t arg_count = argc - optind;
	if ((prompt && arg_count != 1) || (!prompt && arg_count != 2)) {
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
			fprintf(
				stderr, "%s: unsupported key -- '%c'\n", VIRSH_SS, input[i]);
		};
	}

	if (speed > 1) {
		// Fast send if applicable
		uint32_t current = 0;
		while (current < strlen(input)) {
			int is_seq_shifted = is_shifted(input[current]);
			uint32_t search = current + 1;

			// Search until speed reached, end of string or different shifting
			while (search - current < speed && search < strlen(input)
				   && is_shifted(input[search]) == is_seq_shifted) {
				search++;
			}

			// Characters start at index current, end at difference
			if (send_keys(domain, input + current, search - current)
				!= EXIT_SUCCESS) {
				fprintf(stderr, "%s: failed to send keys\n", VIRSH_SS);
				if (current > 0) {
					fprintf(
						stderr, "warning: %u keys have been sent\n", current);
				}

				if (prompt) {
					dispose(input);
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

				if (prompt) {
					dispose(input);
				}
				return EXIT_FAILURE;
			}
		}
	}

	if (newline) {
		if (send_key(domain, '\n') != EXIT_SUCCESS) {
			fprintf(stderr, "%s: failed to send newline\n", VIRSH_SS);
			fprintf(stderr, "warning: string was sent\n");

			if (prompt) {
				dispose(input);
			}
			return EXIT_FAILURE;
		}
	}

	if (prompt) {
		dispose(input);
	}
	return EXIT_SUCCESS;
}

/**
 * @brief Read input from the user.
 *
 * @return Input string (must be freed).
 */
char *get_input(void) {
	char *input;
	nrl_error err;

	if (secret) {
		// Ensure that read is from the terminal
		int tty_fd = open("/dev/tty", O_RDWR);
		if (tty_fd < 0) {
			fprintf(stderr, "%s: failed to open /dev/tty: %s\n", VIRSH_SS,
				strerror(errno));
			exit(EXIT_FAILURE);
		}

		nrl_opts opts = {
			.fd = tty_fd,
			.prompt = "input string: ",
			.echo = NRL_ECHO_FAKE,
			.echo_repl = '*',
		};

		input = nanorl_opts(&opts, &err);
		close(tty_fd);
	} else {
		// Doesn't matter in this case
		input = nanorl("input string: ", &err);
	}

	if (input == NULL) {
		if (err == NRL_ERR_EMPTY) {
			fprintf(stderr, "%s: no input was given\n", VIRSH_SS);
			exit(EXIT_FAILURE);
		} else {
			fprintf(stderr, "%s: failed to read input\n", VIRSH_SS);
			exit(EXIT_FAILURE);
		}
	}

	return input;
}

/**
 * @brief Print usage information.
 */
void print_usage(void) {
	printf("usage: %s [options] <domain> <string>\n", VIRSH_SS);
	printf("%10s - %s\n", "domain", "libvirt domain name");
	printf("%10s - %s\n", "string", "string to send (if prompt not set)");
	printf("options:\n");
	printf("%15s - %s\n", "--help, -h", "show usage information");
	printf("%15s - %s\n", "--version, -v", "show program version");
	printf("%15s - %s\n", "--prompt, -p", "ask for a string as a prompt");
	printf("%15s - %s\n", "--secret, -s", "disable prompt input echo");
	printf(
		"%15s - %s\n", "--newline, -n", "send a newline character at the end");
	printf("%15s - %s\n", "--speed, -l",
		"max amount of characters sent per send-key command (1-15)");
	printf("%15s - %s\n", "", "higher values might cause issues (default: 1)");
}

/**
 * @brief Print version information.
 */
static void print_version(void) {
	printf("%-17s - version %s\n", "Virsh Send String", VIRSH_SS_VERSION);
	printf("%-17s - version %s\n", "Nanorl (lib)", NRL_LIB_VER);

	printf("\nCopyright (C) 2024 Vladyslav Aviedov\n");
	printf("This program is free software released under the GNU GPLv3\n");

#if DEBUG == 1
	printf("\nDebug Build\n");
#endif
}

/**
 *
 * @brief Verify that we can send this key.
 *
 * @param[in] c - Character to check.
 * @return Boolean result.
 */
int verify_key(char c) {
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
int send_key(char *domain, char c) {
	int shifted = is_shifted(c);

	// Make key
	char key_name[32];
	format_key(c, shifted, key_name, 32);

	// virsh send-key DOMAIN (KEY_LEFTSHIFT) KEY NULL
	char *args[5 + shifted];
	args[0] = get_binary_name();
	args[1] = "send-key";
	args[2] = domain;

	if (shifted) {
		args[3] = SHIFT_CMD;
		args[4] = key_name;
		args[5] = NULL;
	} else {
		args[3] = key_name;
		args[4] = NULL;
	}

#if DEBUG == 1
	printf("debug: sending command\n");
	char **args_trav = args;
	while (*args_trav != NULL) {
		printf("\t%s\n", *args_trav);
		args_trav++;
	}
#endif

	return run_virsh(args);
}

/**
 * @brief Send 'count' keys to a libvirt domain.
 *
 * @param[in] domain - Libvirt domain.
 * @param[in] keys - Character array to send.
 * @param[in] count - Amount of characters in 'c'
 * @return [TODO:description]
 */
int send_keys(char *domain, const char *keys, uint32_t count) {
	int shifted = is_shifted(keys[0]);

	// Make keys
	char *key_names[count];
	for (uint32_t i = 0; i < count; i++) {
		key_names[i] = malloc(sizeof(char) * 32);
		format_key(keys[i], shifted, key_names[i], 32);
	}

	// virsh send-key DOMAIN (KEY_LEFTSHIFT) [KEYS] NULL
	char *args[4 + shifted + count];
	args[0] = get_binary_name();
	args[1] = "send-key";
	args[2] = domain;

	if (shifted) {
		args[3] = SHIFT_CMD;
	}

	for (uint32_t i = 0; i < count; i++) {
		args[3 + shifted + i] = key_names[i];
	}
	args[3 + shifted + count] = NULL;

#if DEBUG == 1
	printf("debug: sending command\n");
	char **args_trav = args;
	while (*args_trav != NULL) {
		printf("\t%s\n", *args_trav);
		args_trav++;
	}
#endif

	// Run
	int result = run_virsh(args);

	// Cleanup
	for (uint32_t i = 0; i < count; i++) {
		free(key_names[i]);
	}

	return result;
}

/**
 * @brief Get virsh binary name.
 *
 * @return Binary name (argv0).
 */
static char *get_binary_name(void) {
	char *from_env = getenv("VIRSH");
	return from_env != NULL ? from_env : VIRSH_BIN;
}

/**
 * @brief Check if key requires shift to type.
 *
 * @param[in] c - Character to check.
 * @return Boolean result.
 */
int is_shifted(char c) {
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
void format_key(char c, int shifted, char *buffer, uint32_t buffer_size) {
	// Format normal characters
	if (isupper(c) || islower(c)) {
		snprintf(buffer, buffer_size, "KEY_%c", toupper(c));
		return;
	}

	// Format misc characters
	const char *formatted;
	for (uint32_t i = 0; i < MISC_KEY_COUNT; i++) {
		const kb_key *entry = misc_keys + i;

		if ((!shifted && c == entry->unshifted)
			|| (shifted && c == entry->shifted)) {
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
int run_virsh(char **args) {
	// Fork process
	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "%s: failed to fork process\n", VIRSH_SS);
		return EXIT_FAILURE;
	}

	// Child code
	if (pid == 0) {
		// Redirect output to /dev/null
		int dev_null = open("/dev/null", O_WRONLY);
		dup2(dev_null, STDOUT_FILENO);
		close(dev_null);

		execvp(args[0], args);
		fprintf(stderr, "%s: failed to exec virsh\n", VIRSH_SS);
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

/**
 * @brief Free buffer, erasing data if secret flag is on.
 *
 * @param[in] buffer - String to delete.
 */
static void dispose(char *buffer) {
	if (secret) {
		memset(buffer, 0, strlen(buffer));
	}

	free(buffer);
}
