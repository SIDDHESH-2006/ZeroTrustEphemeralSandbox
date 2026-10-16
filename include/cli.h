#ifndef SANDBOX_CLI_H
#define SANDBOX_CLI_H

/* Include core sandbox structures and configurations */
#include "sandbox.h"

/*
 * cli_print_usage:
 * Prints the complete command-line synopsis, available flags, and usage examples to stdout.
 * @param prog_name: The name of the binary invoked (typically argv[0]).
 */
void cli_print_usage(const char *prog_name);

/*
 * cli_print_version:
 * Prints project metadata, course information, student authors, and faculty guide to stdout.
 */
void cli_print_version(void);

/*
 * cli_parse_args:
 * Parses command-line arguments using getopt_long, validates paths, and extracts payload command.
 * @param argc: Argument count from main().
 * @param argv: Argument vector from main().
 * @param config: Pointer to sandbox_config structure to populate with parsed options.
 * @return 0 on success, or -1 if an error or invalid option is encountered.
 */
int cli_parse_args(int argc, char *argv[], struct sandbox_config *config);

/*
 * cli_add_default_paths:
 * Automatically allowlists standard system paths required for dynamic linking and standard binaries.
 * @param config: Pointer to sandbox_config to append default path rules to.
 * @return 0 on success, or -1 if the path rule limit is exceeded.
 */
int cli_add_default_paths(struct sandbox_config *config);

#endif /* SANDBOX_CLI_H */
