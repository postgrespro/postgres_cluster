#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

#define MAX_CMD_LEN 20

#define PARSER_STATE_ERROR  -1
#define PARSER_STATE_INITIAL 0
#define PARSER_STATE_PREPARE 1
#define PARSER_STATE_COMMIT  2
#define PARSER_STATE_ABORT   3
#define PARSER_STATE_HORIZON 4
#define PARSER_STATE_STATUS  5

#define PARSER_STATE_FIRST   1
#define PARSER_STATE_LAST    5

char *syntax[6] = {
	"",                  // initial
	"p",                 // prepare
	"chhhhhhhhhhhhhhhh", // commit
	"ahhhhhhhhhhhhhhhh", // abort
	"h",                 // horizon
	"shhhhhhhhhhhhhhhh", // status
};

typedef struct parser_data_t {
	int state;
	char buf[MAX_CMD_LEN];
	int bufusage;
	char *errormsg;
} parser_data_t;

// Allocate and initialize a parser.
parser_t parser_create() {
	parser_t p = malloc(sizeof(parser_data_t));
	parser_init(p);
	return p;
}

// Destroy the parser. The 'p' handle becomes invalid, so do not refer to it
// after destroying the parser.
void parser_destroy(parser_t p) {
	free(p);
}

// Initialize the parser.
void parser_init(parser_t p) {
	p->state = PARSER_STATE_INITIAL;
	p->bufusage = 0;
	p->errormsg = NULL;
}

// Check if parser has failed.
bool parser_failed(parser_t p) {
	return p->state == PARSER_STATE_ERROR;
}

// Get the error message for the parser.
char *parser_errormsg(parser_t p) {
	return p->errormsg;
}

static bool is_hex_digit(char c) {
	if ((c >= '0') && (c <= '9')) {
		return true;
	}
	if ((c >= 'a') && (c <= 'f')) {
		return true;
	}
	return false;
}

// Checks if the command is complete. If it is, returns the command and
// initializes the parser. Does nothing and returns NULL otherwise.
static cmd_t *parser_finish_if_possible(parser_t p) {
	if (p->state == PARSER_STATE_ERROR) {
			return NULL;
	}
	if (p->state == PARSER_STATE_INITIAL) {
			return NULL;
	}

	if (syntax[p->state][p->bufusage] == '\0') {
		// finish the command
		cmd_t *cmd = malloc(sizeof(cmd_t));

		cmd->cmd = syntax[p->state][0];
		if (p->bufusage > 16) {
			sscanf(p->buf + 1, "%016llx", &(cmd->arg));
		} else {
			cmd->arg = 0;
		}

		parser_init(p);
		return cmd;
	}

	return NULL;
}

// Feeds a character to the parser, and returns a parsed command if it is
// complete. Returns NULL if command is not complete. The caller should check
// for errors, please use parser_failed() method for that. Also the caller
// should free the cmd after use.
cmd_t *parser_feed(parser_t p, char c) {
	if (p->state == PARSER_STATE_ERROR) {
		return NULL;
	}

	if (p->state == PARSER_STATE_INITIAL) {
		int next;
		for (next = PARSER_STATE_FIRST; next <= PARSER_STATE_LAST; next++) {
			if (syntax[next][0] == c) {
				p->state = next;
				p->buf[p->bufusage++] = c;
				return parser_finish_if_possible(p);
			}
		}
		p->state = PARSER_STATE_ERROR;
		p->errormsg = "unsupported command";
		return NULL;
	}

	if (syntax[p->state][p->bufusage] == 'h') { // check if input is a hex digit
		if (!is_hex_digit(c)) {
			p->state = PARSER_STATE_ERROR;
			p->errormsg = "not a hex digit";
			return NULL;
		}
	} else {
		p->state = PARSER_STATE_ERROR;
		p->errormsg = "internal error";
		return NULL;
	}

	p->buf[p->bufusage++] = c;
	return parser_finish_if_possible(p);
}
