#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "parser.h"
#include "transaction.h"

// cmd(1), argc(16), argv[2*n](len * 16), null(1)
#define MAX_CMD_LEN (1 + (1 + 2 * MAX_NODES) * 16 + 1)

#define PARSER_STATE_ERROR   -1
#define PARSER_STATE_INITIAL  0
#define PARSER_STATE_ARGC     1
#define PARSER_STATE_ARGV     2
#define PARSER_STATE_COMPLETE 3

typedef struct parser_data_t {
	int state;
	cmd_t *cmd;
	int args;
	int digits;
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
	free(p->cmd);
	free(p);
}

// Initialize the parser.
void parser_init(parser_t p) {
	p->state = PARSER_STATE_INITIAL;
	p->cmd = malloc(sizeof(cmd_t));
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

static int unhex_digit(char c) {
	assert(is_hex_digit(c));
	if ((c >= '0') && (c <= '9')) {
		return c - '0';
	}
	if ((c >= 'a') && (c <= 'f')) {
		return c - 'a' + 10;
	}
	return -1;
}

// Checks if the command is complete. If it is, returns the command and
// initializes the parser. Does nothing and returns NULL otherwise.
static cmd_t *parser_finish_if_possible(parser_t p) {
	if (p->state != PARSER_STATE_COMPLETE) {
			return NULL;
	}

	cmd_t *cmd = p->cmd;
	parser_init(p);
	return cmd;
}

// Feeds a character to the parser, and returns a parsed command if it is
// complete. Returns NULL if command is not complete. The caller should check
// for errors, please use parser_failed() method for that. Also the caller
// should free the cmd after use.
cmd_t *parser_feed(parser_t p, char c) {
	if (p->state == PARSER_STATE_ERROR) {
		return NULL;
	}

	if (p->state == PARSER_STATE_COMPLETE) {
		p->state = PARSER_STATE_ERROR;
		p->errormsg = "unexpected data after a command";
		return NULL;
	}

	if (p->state == PARSER_STATE_INITIAL) {
		p->cmd->cmd = c;
		p->state = PARSER_STATE_ARGC;
		p->args = 0;
		p->digits = 0;
		return NULL;
	}

	if (p->state == PARSER_STATE_ARGC) {
		if (!is_hex_digit(c)) {
			p->state = PARSER_STATE_ERROR;
			p->errormsg = "not a hex digit";
			return NULL;
		}
		p->cmd->argc *= 16;
		p->cmd->argc += unhex_digit(c);
		if (++(p->digits) == 16) {
			if (p->cmd->argc == 0) {
				p->state = PARSER_STATE_COMPLETE;
				return parser_finish_if_possible(p);
			}
			p->state = PARSER_STATE_ARGV;
			p->args = 0;
			p->digits = 0;
		}
		return NULL;
	}

	if (p->state == PARSER_STATE_ARGV) {
		if (!is_hex_digit(c)) {
			p->state = PARSER_STATE_ERROR;
			p->errormsg = "not a hex digit";
			return NULL;
		}
		p->cmd->argv[p->args] *= 16;
		p->cmd->argv[p->args] += unhex_digit(c);
		if (++(p->digits) == 16) {
			p->state = PARSER_STATE_ARGV;
			p->args++;
			p->digits = 0;
			if (p->cmd->argc == p->args) {
				p->state = PARSER_STATE_COMPLETE;
				return parser_finish_if_possible(p);
			}
		}
		return NULL;
	}

	return parser_finish_if_possible(p);
}
