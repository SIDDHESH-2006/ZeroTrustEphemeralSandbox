#ifndef SANDBOX_CLI_H
#define SANDBOX_CLI_H

#include "sandbox.h"

void cli_print_usage(const char *prog_name);
void cli_print_version(void);
int cli_parse_args(int argc, char *argv[], struct sandbox_config *config);
int cli_add_default_paths(struct sandbox_config *config);

#endif /* SANDBOX_CLI_H */
