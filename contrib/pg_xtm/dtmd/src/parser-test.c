#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

char *input = 
	"b"
	"cdeadbeefcafebabe"
	"adeadbeefcafebabe"
	"h"
	"sdeadbeefcafebabe"
;

#define NUM_EXPECTED 5
cmd_t expected[NUM_EXPECTED] = {
	{CMD_BEGIN, 0UL},
	{CMD_COMMIT, 0xdeadbeefcafebabeUL},
	{CMD_ABORT, 0xdeadbeefcafebabeUL},
	{CMD_SNAPSHOT, 0UL},
	{CMD_STATUS, 0xdeadbeefcafebabeUL},
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
			printf("    arg: %016llx (%s)\n", cmd->arg, cmd->arg == expected[cmds].arg ? "ok" : "FAILURE");
			if ((cmd->cmd != expected[cmds].cmd) || (cmd->arg != expected[cmds].arg)) {
				ok = false;
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
