/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <lib/console.h>

static cmd_block *command_list = NULL;

/* a linear array of statically defined command blocks,
   defined in the linker script.
 */
extern cmd_block __commands_start;
extern cmd_block __commands_end;

static int cmd_help(int argc, const cmd_args *argv);
static int cmd_test(int argc, const cmd_args *argv);

STATIC_COMMAND_START
	{ "help", "this list", &cmd_help },
	{ "test", "test the command processor", &cmd_test },
STATIC_COMMAND_END(help); 

int console_init(void)
{
	printf("console_init: entry\n");

	/* add all the statically defined commands to the list */
	cmd_block *block;
	for (block = &__commands_start; block != &__commands_end; block++) {
		console_register_commands(block);
	}
	
	return 0;
}

static const cmd *match_command(const char *command)
{
	cmd_block *block;
	size_t i;

	for (block = command_list; block != NULL; block = block->next) {
		const cmd *curr_cmd = block->list;
		for (i = 0; i < block->count; i++) {
			if (strcmp(command, curr_cmd[i].cmd_str) == 0) {
				return &curr_cmd[i];
			}
		}
	}

	return NULL;
}

static int read_line(char *buffer, int len)
{
	int pos = 0;
	int escape_level = 0;

	for (;;) {
		char c;

		/* loop until we get a char */
		if (getc(&c) < 0)
			continue;

//		printf("c = 0x%hhx\n", c); 
		
		if (escape_level == 0) {
			switch (c) {
				case '\r':
				case '\n':
					putc(c);
					goto done;

				case 0x7f: // backspace or delete
				case 0x8:
					if (pos > 0) {
						pos--;
						puts("\x1b[1D"); // move to the left one
						putc(' ');
						puts("\x1b[1D"); // move to the left one
					}
					break;

				case 0x1b: // escape
					escape_level++;
					break;

				default:
					buffer[pos++] = c;
					putc(c);
			}
		} else if (escape_level == 1) {
			// inside an escape, look for '['
			if (c == '[') {
				escape_level++;
			} else {
				// we didn't get it, abort
				escape_level = 0;
			}
		} else { // escape_level > 1
			switch (c) {
				case 67: // right arrow
					buffer[pos++] = ' ';
					putc(' ');
					break;
				case 68: // left arrow
					if (pos > 0) {
						pos--;
						puts("\x1b[1D"); // move to the left one
						putc(' ');
						puts("\x1b[1D"); // move to the left one
					}
					break;
				case 65: // up arrow
				case 66: // down arrow
					// XXX do history here
					break;
				default:
					break;
			}
			escape_level = 0;
		}

		/* end of line. */
		if (pos == (len - 1)) {
			puts("\nerror: line too long\n");
			pos = 0;
			goto done;
		}
	}

done:
//	printf("returning pos %d\n", pos);

	buffer[pos] = 0;
	return pos;
}

static int tokenize_command(char *buffer, cmd_args *args, int arg_count)
{
	int pos;
	int arg;
	bool finished;
	enum {
		INITIAL = 0,
		IN_SPACE,
		IN_TOKEN
	} state;

	pos = 0;
	arg = 0;
	state = INITIAL;
	finished = false;

	for (;;) {
		char c = buffer[pos];

		if (c == '\0')
			finished = true;

//		printf("c 0x%hhx state %d arg %d pos %d\n", c, state, arg, pos);

		switch (state) {
			case INITIAL:
				if (isspace(c)) {
					state = IN_SPACE;
				} else {
					state = IN_TOKEN;
					args[arg].str = &buffer[pos];
				}
				break;
			case IN_TOKEN:
				if (finished) {
					arg++;
					goto done;
				}
				if (isspace(c)) {
					arg++;					
					buffer[pos] = 0;			
					/* are we out of tokens? */
					if (arg == arg_count)
						goto done;
					state = IN_SPACE;
				}
				pos++;
				break;
			case IN_SPACE:
				if (finished)
					goto done;
				if (!isspace(c)) {
					state = IN_TOKEN;
					args[arg].str = &buffer[pos];
				}
				pos++;
				break;
		}
	}

done:
	return arg;
}

static void convert_args(int argc, cmd_args *argv)
{
	int i;

	for (i = 0; i < argc; i++) {
		argv[i].u = atoui(argv[i].str);
		argv[i].i = atoi(argv[i].str);
	}
}

static void console_loop(void)
{
	cmd_args args[16];
	char buffer[256];

	printf("entering main console loop\n");

	for (;;) {
		puts("] ");

		int len = read_line(buffer, sizeof(buffer));
		if (len == 0)
			continue;

//		printf("line = '%s'\n", buffer);

		/* tokenize the line */
		int argc = tokenize_command(buffer, args, 16);
		if (argc < 0) {
			printf("syntax error\n");
			continue;
		} else if (argc == 0) {
			continue;
		}

//		printf("after tokenize: argc %d\n", argc);
//		for (int i = 0; i < argc; i++)
//			printf("%d: '%s'\n", i, args[i].str);

		/* convert the args */
		convert_args(argc, args);

		/* try to match the command */
		const cmd *command = match_command(args[0].str);
		if (!command) {
			printf("command not found\n");
			continue;
		}

		int result = command->cmd_callback(argc, args);

		// XXX do something with the result
	}
}


void console_start(void)
{

	console_loop();
}

int console_run_command(const char *string)
{
	const cmd *command;

	ASSERT(string != NULL);

	command = match_command(string);
	if (!command)
		return -1;

	int result = command->cmd_callback(0, NULL);

	return result;
}

void console_register_commands(cmd_block *block)
{
	ASSERT(block);
	ASSERT(block->next == NULL);

	block->next = command_list;
	command_list = block;
}

static int cmd_help(int argc, const cmd_args *argv)
{

	printf("command list:\n");
	
	cmd_block *block;
	size_t i;

	for (block = command_list; block != NULL; block = block->next) {
		const cmd *curr_cmd = block->list;
		for (i = 0; i < block->count; i++) {
			printf("\t%-16s: %s\n", curr_cmd[i].cmd_str, curr_cmd[i].help_str ? curr_cmd[i].help_str : "");
		}
	}

	return 0;
}

static int cmd_test(int argc, const cmd_args *argv)
{
	int i;

	printf("argc %d, argv %p\n", argc, argv);
	for (i = 0; i < argc; i++)
		printf("\t%d: str '%s', i %d, u %#x\n", i, argv[i].str, argv[i].i, argv[i].u);

	return 0;
}

