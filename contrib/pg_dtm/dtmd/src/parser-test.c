#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

char *input = 
	"a0000000000000001000000000000002a"
	"b0000000000000002000000000000002b000000000000002c"
	"z0000000000000003000000000000002d000000000000002e000000000000002f"
;

#define NUM_EXPECTED 3
cmd_t expected[NUM_EXPECTED] = {
	{'a', 1, {42}},
	{'b', 2, {43, 44}},
	{'z', 3, {45, 46, 47}},
};

int main() {
	parser_t parser = parser_create();

	bool ok = true;
	int cmds = 0;
	char *c;
	for (c = input; *c; c++) {
		if (*c == '\n') {
			// ignore newlines for testing
			continue;
		}

		cmd_t *cmd = parser_feed(parser, *c);
		if (parser_failed(parser)) {
			fprintf(stderr, "parser failed: %s\n", parser_errormsg(parser));
			ok = false;
			break;
		}
		if (cmd) {
			if (cmds >= NUM_EXPECTED) {
				fprintf(stderr, "an unexpected command parsed\n");
				ok = false;
				break;
			}
			printf("command: %c (%s)\n", cmd->cmd, cmd->cmd == expected[cmds].cmd ? "ok" : "FAILURE");
			printf("   argc: %d (%s)\n", cmd->argc, cmd->argc == expected[cmds].argc ? "ok" : "FAILURE");

			if ((cmd->cmd != expected[cmds].cmd) || (cmd->argc != expected[cmds].argc)) {
				ok = false;
			}

			int i;
			for (i = 0; (i < cmd->argc) && (i < expected[cmds].argc); i++) {
				printf("argv[%d]: %llu (%s)\n", i, cmd->argv[i], cmd->argv[i] == expected[cmds].argv[i] ? "ok" : "FAILURE");
				if (cmd->argv[i] != expected[cmds].argv[i]) {
					ok = false;
				}
			}
			cmds++;
			free(cmd);
		}
	}

	parser_destroy(parser);

	if (ok) {
		printf("parser-test passed\n");
		return EXIT_SUCCESS;
	} else {
		printf("parser-test FAILED\n");
		return EXIT_FAILURE;
	}
}
