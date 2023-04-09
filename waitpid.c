#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef WAITPID_VERSION
#define WAITPID_VERSION "unknown"
#endif

#ifndef EXIT_FAULTY
#define EXIT_FAULTY 2
#endif

#ifndef WAITPID_DEFAULT_TIMEOUT_EXIT
#define WAITPID_DEFAULT_TIMEOUT_EXIT 124
#endif

static const char* program_name = "waitpid";

/*
 * Prints usage of the software. Called either as part of print_help,
 * or when the user entered command-line invocation was faulty.
 */
static void print_usage(void) {
	fprintf(stderr, "Usage: %s [OPTIONS] <PID>\n", program_name);
	fprintf(stderr, "  -t, --timeout=<SECONDS>       Timeout in seconds (default -1)\n");
	fprintf(stderr, "  -s, --timeout-status=<STATUS> Exit status when timeout occurs (default 124)\n");
	fprintf(stderr, "  -v, --verbose                 Increase verbosity (can be used multiple times)\n");
	fprintf(stderr, "  -h, --help                    Display this help message and exit\n");
	fprintf(stderr, "  -V, --version                 Display version information and exit\n");
}

/*
 * Prints the help of the software. Called when the user invokes the
 * software with --help or -h.
 */
static void print_help(void) {
	fprintf(stderr, "Wait for a foreign process to exit.\n");
	fprintf(stderr, "\n");
	print_usage();
	fprintf(stderr, "\n");
	fprintf(stderr, "Exit status:\n");
	fprintf(stderr, "  %d: Process exited as expected.\n", EXIT_SUCCESS);
	fprintf(stderr, "  %d: The specified <PID> was invalid or other errors occurred.\n", EXIT_FAILURE);
	fprintf(stderr, "  %d: The command-line invocation of %s was faulty.\n", EXIT_FAULTY, program_name);
	fprintf(stderr, "  %d (unless changed with --timeout-status): Timeout occurred.\n", WAITPID_DEFAULT_TIMEOUT_EXIT);
}

/*
 * Prints the version and licensing information of the software. Called
 * when the user invokes the software with --version or -V.
 */
static void print_version(void) {
	fprintf(stderr, "%s (version %s)\n", program_name, WAITPID_VERSION);
	fprintf(stderr, "Copyright (c) 2023 Florian \"sp1rit\" <sp1rit@national.shitposting.agency>\n");
	fprintf(stderr, "\nLicensed under the GNU General Public License version 3 or later.\n");
	fprintf(stderr, "  You should have received a copy of it along with this program.\n");
	fprintf(stderr, "  If not, see <https://www.gnu.org/licenses/>.\n\n");
	fprintf(stderr, "This is free software: you are free to change and redistribute it.\n");
	fprintf(stderr, "This program comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n");
}

unsigned char verbosity = 0;
/*
 * Custom fprintf function that only prints if the needed verbosity is met.
 */
static void printf_vf(FILE* stream, unsigned char needed_verbosity, const char* format, ...) {
	if (verbosity >= needed_verbosity ) {
		va_list args;
		va_start(args, format);
		vfprintf(stream, format, args);
		va_end(args);
	}
}

/*
 * libc entrypoint
 */
int main(int argc, char** argv) {
	// Default timeout and timeout status values
	int timeout = -1;
	int timeout_status = WAITPID_DEFAULT_TIMEOUT_EXIT;
	pid_t pid = -1;

	static struct option long_options[] = {
		{"timeout", required_argument, 0, 't'},
		{"timeout-status", required_argument, 0, 's'},
		{"verbose", no_argument, 0, 'v'},
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'V'},
		{0, 0, 0, 0}
	};

	// If there is the first command-line argument (this should be the
	// invocation itself), set program_name to it. This better reflects
	// the software on the users OS.
	if (argc > 0)
		program_name = argv[0];

	int c;
	int option_idx = 0;
	while ((c = getopt_long(argc, argv, "t:s:vhV", long_options, &option_idx)) != -1) {
		switch (c) {
			// poll() takes timeout in milliseconds, take the user
			// specified timeout as floating point and multiply by 10^3
			// and cast back to int of milliseconds. That should be
			// accurrate enough.
			case 't': {
				float user_timeout = strtof(optarg, NULL);
				timeout = user_timeout * 1000.f;
				} break;
			// Convert the timeout status argumet to an integer and
			// store it.
			case 's':
				timeout_status = atoi(optarg);
				break;
			// Increase verbosity by one.
			case 'v':
				verbosity++;
				break;
			// Print help message and exit.
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			// Print version and licensing information and exit.
			case 'V':
				print_version();
				return EXIT_SUCCESS;
			// Invalid input. Print usage message and exit.
			default:
				print_usage();
				return EXIT_FAULTY;
		}
	}

	// Parse the positional argument
	if (optind < argc) {
		pid = atoi(argv[optind]);
	} else {
		// Print usage message and exit if there's no pid argument.
		print_usage();
		return EXIT_FAULTY;
	}

	// Print debug message with process ID being looked for.
	printf_vf(stderr, 1, "Looking for process with pid %d.\n", pid);

	// Open process file descriptor using pidfd_open syscall
	errno = 0;
	int pfd = syscall(SYS_pidfd_open, pid, 0);
	if (pfd == -1) {
		// If process file descriptor cannot be opened, print error
		// message and exit.
		int error = errno;
		fprintf(
			stderr,
			"Failed looking for process %d: %s (Code %d)\n",
			pid, strerror(error), error
		);
		return EXIT_FAILURE;
	}
	int exit_code = EXIT_SUCCESS;

	// Print debug message with process file descriptor being used.
	printf_vf(stderr, 1, "Found process, now open as fd %d.\n", pfd);

	// Use poll to wait for the process to exit or a timeout to occur
	struct pollfd fds[1];
	fds[0].fd = pfd;
	fds[0].events = POLLIN;

	int ret = poll(fds, 1, timeout);
	if (ret < 0) {
		// If poll returns an error, print error message and exit.
		int error = errno;
		fprintf(
			stderr,
			"Failed polling process %d (open as fd %d): %s (Code %d)\n",
			pid, pfd, strerror(error), error
		);
		exit_code = EXIT_FAILURE;
		goto exit;
	}
	if (ret == 0) {
		// If poll timed out, print debug message and exit with
		// timeout status.
		printf_vf(stderr, 1, "Poll timeout ocurred, exiting with timeout status.\n");
		exit_code = timeout_status;
		goto exit;
	}

	if (fds[0].revents & POLLIN) {
		// If POLLIN event occurred, print debug message.
		printf_vf(stderr, 1, "Process exited as expected, exiting self.\n");
	} else {
		// If unexpected event occurred, print error message.
		fprintf(stderr, "Unexpected event received: %#x\n", fds[0].revents);
		exit_code = EXIT_FAILURE;
	}

	// Close process file descriptor and return with appropriate
	// status.
exit:
	close(pfd);
	return exit_code;
}
