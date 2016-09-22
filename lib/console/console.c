/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
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
#include <trace.h>
#include <assert.h>
#include <err.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <lib/console.h>
#if WITH_LIB_ENV
#include <lib/env.h>
#endif

#ifndef CONSOLE_ENABLE_HISTORY
#define CONSOLE_ENABLE_HISTORY 1
#endif

// Whether to enable "repeat" command.
#ifndef CONSOLE_ENABLE_REPEAT
#define CONSOLE_ENABLE_REPEAT 1
#endif

#define LINE_LEN 128

#define PANIC_LINE_LEN 32

#define MAX_NUM_ARGS 16

#define HISTORY_LEN 16

#define LOCAL_TRACE 0

#define WHITESPACE " \t"

/* debug buffer */
static char *debug_buffer;

/* echo commands? */
static bool echo = true;

/* command processor state */
static mutex_t *command_lock;
int lastresult;
static bool abort_script;

#if CONSOLE_ENABLE_HISTORY
/* command history stuff */
static char *history; // HISTORY_LEN rows of LINE_LEN chars a piece
static uint history_next;

static void init_history(void);
static void add_history(const char *line);
static uint start_history_cursor(void);
static const char *next_history(uint *cursor);
static const char *prev_history(uint *cursor);
static void dump_history(void);
#endif

/* list of installed commands */
static cmd_block *command_list = NULL;

/* a linear array of statically defined command blocks,
   defined in the linker script.
 */
extern cmd_block __commands_start;
extern cmd_block __commands_end;

static int cmd_help(int argc, const cmd_args *argv);
static int cmd_help_panic(int argc, const cmd_args *argv);
static int cmd_echo(int argc, const cmd_args *argv);
static int cmd_test(int argc, const cmd_args *argv);
#if CONSOLE_ENABLE_HISTORY
static int cmd_history(int argc, const cmd_args *argv);
#endif
#if CONSOLE_ENABLE_REPEAT
static int cmd_repeat(int argc, const cmd_args *argv);
#endif

STATIC_COMMAND_START
STATIC_COMMAND("help", "this list", &cmd_help)
STATIC_COMMAND_MASKED("help", "this list", &cmd_help_panic, CMD_AVAIL_PANIC)
STATIC_COMMAND("echo", NULL, &cmd_echo)
#if LK_DEBUGLEVEL > 1
STATIC_COMMAND("test", "test the command processor", &cmd_test)
#if CONSOLE_ENABLE_HISTORY
STATIC_COMMAND("history", "command history", &cmd_history)
#endif
#if CONSOLE_ENABLE_REPEAT
STATIC_COMMAND("repeat", "repeats command multiple times", &cmd_repeat)
#endif
#endif
STATIC_COMMAND_END(help);

int console_init(void)
{
    LTRACE_ENTRY;

    command_lock = calloc(sizeof(mutex_t), 1);
    mutex_init(command_lock);

    /* add all the statically defined commands to the list */
    cmd_block *block;
    for (block = &__commands_start; block != &__commands_end; block++) {
        console_register_commands(block);
    }

#if CONSOLE_ENABLE_HISTORY
    init_history();
#endif

    return 0;
}

#if CONSOLE_ENABLE_HISTORY
static int cmd_history(int argc, const cmd_args *argv)
{
    dump_history();
    return 0;
}

static inline char *history_line(uint line)
{
    return history + line * LINE_LEN;
}

static inline uint ptrnext(uint ptr)
{
    return (ptr + 1) % HISTORY_LEN;
}

static inline uint ptrprev(uint ptr)
{
    return (ptr - 1) % HISTORY_LEN;
}

static void dump_history(void)
{
    printf("command history:\n");
    uint ptr = ptrprev(history_next);
    int i;
    for (i=0; i < HISTORY_LEN; i++) {
        if (history_line(ptr)[0] != 0)
            printf("\t%s\n", history_line(ptr));
        ptr = ptrprev(ptr);
    }
}

static void init_history(void)
{
    /* allocate and set up the history buffer */
    history = calloc(1, HISTORY_LEN * LINE_LEN);
    history_next = 0;
}

static void add_history(const char *line)
{
    // reject some stuff
    if (line[0] == 0)
        return;

    uint last = ptrprev(history_next);
    if (strcmp(line, history_line(last)) == 0)
        return;

    strlcpy(history_line(history_next), line, LINE_LEN);
    history_next = ptrnext(history_next);
}

static uint start_history_cursor(void)
{
    return ptrprev(history_next);
}

static const char *next_history(uint *cursor)
{
    uint i = ptrnext(*cursor);

    if (i == history_next)
        return ""; // can't let the cursor hit the head

    *cursor = i;
    return history_line(i);
}

static const char *prev_history(uint *cursor)
{
    uint i;
    const char *str = history_line(*cursor);

    /* if we are already at head, stop here */
    if (*cursor == history_next)
        return str;

    /* back up one */
    i = ptrprev(*cursor);

    /* if the next one is gonna be null */
    if (history_line(i)[0] == '\0')
        return str;

    /* update the cursor */
    *cursor = i;
    return str;
}
#endif  // CONSOLE_ENABLE_HISTORY

#if CONSOLE_ENABLE_REPEAT
static int cmd_repeat(int argc, const cmd_args* argv)
{
    if (argc < 4) goto usage;
    int times = argv[1].i;
    int delay = argv[2].i;
    if (times <= 0) goto usage;
    if (delay < 0) goto usage;

    // Worst case line length with quoting.
    char line[LINE_LEN + MAX_NUM_ARGS * 3];

    // Paste together all arguments, and quote them.
    int idx = 0;
    for (int i = 3; i < argc; ++i) {
        if (i != 3) {
            // Add a space before all args but the first.
            line[idx++] = ' ';
        }
        line[idx++] = '"';
        for (const char* src = argv[i].str; *src != '\0'; src++) {
            line[idx++] = *src;
        }
        line[idx++] = '"';
    }
    line[idx] = '\0';

    for (int i = 0; i < times; ++i) {
        printf("[%d/%d]\n", i + 1, times);
        int result = console_run_script_locked(line);
        if (result != 0) {
            printf("terminating repeat loop, command exited with status %d\n",
                    result);
            return result;
        }
        thread_sleep(delay);
    }
    return NO_ERROR;

usage:
    printf("Usage: repeat <times> <delay in ms> <cmd> [args..]\n");
    return ERR_INVALID_ARGS;
}
#endif  // CONSOLE_ENABLE_REPEAT

static const cmd *match_command(const char *command, const uint8_t availability_mask)
{
    cmd_block *block;
    size_t i;

    for (block = command_list; block != NULL; block = block->next) {
        const cmd *curr_cmd = block->list;
        for (i = 0; i < block->count; i++) {
            if ((availability_mask & curr_cmd[i].availability_mask) == 0) {
                continue;
            }
            if (strcmp(command, curr_cmd[i].cmd_str) == 0) {
                return &curr_cmd[i];
            }
        }
    }

    return NULL;
}

static int read_debug_line(const char **outbuffer, void *cookie)
{
    int pos = 0;
    int escape_level = 0;
#if CONSOLE_ENABLE_HISTORY
    uint history_cursor = start_history_cursor();
#endif

    char *buffer = debug_buffer;

    for (;;) {
        /* loop until we get a char */
        int c;
        if ((c = getchar()) < 0)
            continue;

//      TRACEF("c = 0x%hhx\n", c);

        if (escape_level == 0) {
            switch (c) {
                case '\r':
                case '\n':
                    if (echo)
                        putchar('\n');
                    goto done;

                case 0x7f: // backspace or delete
                case 0x8:
                    if (pos > 0) {
                        pos--;
                        fputs("\b \b", stdout); // wipe out a character
                    }
                    break;

                case 0x1b: // escape
                    escape_level++;
                    break;

                default:
                    buffer[pos++] = c;
                    if (echo)
                        putchar(c);
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
                    if (echo)
                        putchar(' ');
                    break;
                case 68: // left arrow
                    if (pos > 0) {
                        pos--;
                        if (echo) {
                            fputs("\b \b", stdout); // wipe out a character
                        }
                    }
                    break;
#if CONSOLE_ENABLE_HISTORY
                case 65: // up arrow -- previous history
                case 66: // down arrow -- next history
                    // wipe out the current line
                    while (pos > 0) {
                        pos--;
                        if (echo) {
                            fputs("\b \b", stdout); // wipe out a character
                        }
                    }

                    if (c == 65)
                        strlcpy(buffer, prev_history(&history_cursor), LINE_LEN);
                    else
                        strlcpy(buffer, next_history(&history_cursor), LINE_LEN);
                    pos = strlen(buffer);
                    if (echo)
                        fputs(buffer, stdout);
                    break;
#endif
                default:
                    break;
            }
            escape_level = 0;
        }

        /* end of line. */
        if (pos == (LINE_LEN - 1)) {
            fputs("\nerror: line too long\n", stdout);
            pos = 0;
            goto done;
        }
    }

done:
//  dprintf("returning pos %d\n", pos);

    // null terminate
    buffer[pos] = 0;

#if CONSOLE_ENABLE_HISTORY
    // add to history
    add_history(buffer);
#endif

    // return a pointer to our buffer
    *outbuffer = buffer;

    return pos;
}

static int tokenize_command(const char *inbuffer, const char **continuebuffer, char *buffer, size_t buflen, cmd_args *args, int arg_count)
{
    int inpos;
    int outpos;
    int arg;
    enum {
        INITIAL = 0,
        NEXT_FIELD,
        SPACE,
        IN_SPACE,
        TOKEN,
        IN_TOKEN,
        QUOTED_TOKEN,
        IN_QUOTED_TOKEN,
        VAR,
        IN_VAR,
        COMMAND_SEP,
    } state;
    char varname[128];
    int varnamepos;

    inpos = 0;
    outpos = 0;
    arg = 0;
    varnamepos = 0;
    state = INITIAL;
    *continuebuffer = NULL;

    for (;;) {
        char c = inbuffer[inpos];

//      dprintf(SPEW, "c 0x%hhx state %d arg %d inpos %d pos %d\n", c, state, arg, inpos, outpos);

        switch (state) {
            case INITIAL:
            case NEXT_FIELD:
                if (c == '\0')
                    goto done;
                if (isspace(c))
                    state = SPACE;
                else if (c == ';')
                    state = COMMAND_SEP;
                else
                    state = TOKEN;
                break;
            case SPACE:
                state = IN_SPACE;
                break;
            case IN_SPACE:
                if (c == '\0')
                    goto done;
                if (c == ';') {
                    state = COMMAND_SEP;
                } else if (!isspace(c)) {
                    state = TOKEN;
                } else {
                    inpos++; // consume the space
                }
                break;
            case TOKEN:
                // start of a token
                DEBUG_ASSERT(c != '\0');
                if (c == '"') {
                    // start of a quoted token
                    state = QUOTED_TOKEN;
                } else if (c == '$') {
                    // start of a variable
                    state = VAR;
                } else {
                    // regular, unquoted token
                    state = IN_TOKEN;
                    args[arg].str = &buffer[outpos];
                }
                break;
            case IN_TOKEN:
                if (c == '\0') {
                    arg++;
                    goto done;
                }
                if (isspace(c) || c == ';') {
                    arg++;
                    buffer[outpos] = 0;
                    outpos++;
                    /* are we out of tokens? */
                    if (arg == arg_count)
                        goto done;
                    state = NEXT_FIELD;
                } else {
                    buffer[outpos] = c;
                    outpos++;
                    inpos++;
                }
                break;
            case QUOTED_TOKEN:
                // start of a quoted token
                DEBUG_ASSERT(c == '"');

                state = IN_QUOTED_TOKEN;
                args[arg].str = &buffer[outpos];
                inpos++; // consume the quote
                break;
            case IN_QUOTED_TOKEN:
                if (c == '\0') {
                    arg++;
                    goto done;
                }
                if (c == '"') {
                    arg++;
                    buffer[outpos] = 0;
                    outpos++;
                    /* are we out of tokens? */
                    if (arg == arg_count)
                        goto done;

                    state = NEXT_FIELD;
                }
                buffer[outpos] = c;
                outpos++;
                inpos++;
                break;
            case VAR:
                DEBUG_ASSERT(c == '$');

                state = IN_VAR;
                args[arg].str = &buffer[outpos];
                inpos++; // consume the dollar sign

                // initialize the place to store the variable name
                varnamepos = 0;
                break;
            case IN_VAR:
                if (c == '\0' || isspace(c) || c == ';') {
                    // hit the end of variable, look it up and stick it inline
                    varname[varnamepos] = 0;
#if WITH_LIB_ENV
                    int rc = env_get(varname, &buffer[outpos], buflen - outpos);
#else
                    (void)varname[0]; // nuke a warning
                    int rc = -1;
#endif
                    if (rc < 0) {
                        buffer[outpos++] = '0';
                        buffer[outpos++] = 0;
                    } else {
                        outpos += strlen(&buffer[outpos]) + 1;
                    }
                    arg++;
                    /* are we out of tokens? */
                    if (arg == arg_count)
                        goto done;

                    state = NEXT_FIELD;
                } else {
                    varname[varnamepos] = c;
                    varnamepos++;
                    inpos++;
                }
                break;
            case COMMAND_SEP:
                // we hit a ;, so terminate the command and pass the remainder of the command back in continuebuffer
                DEBUG_ASSERT(c == ';');

                inpos++; // consume the ';'
                *continuebuffer = &inbuffer[inpos];
                goto done;
        }
    }

done:
    buffer[outpos] = 0;
    return arg;
}

static void convert_args(int argc, cmd_args *argv)
{
    int i;

    for (i = 0; i < argc; i++) {
        unsigned long u = atoul(argv[i].str);
        argv[i].u = u;
        argv[i].p = (void *)u;
        argv[i].i = atol(argv[i].str);

        if (!strcmp(argv[i].str, "true") || !strcmp(argv[i].str, "on")) {
            argv[i].b = true;
        } else if (!strcmp(argv[i].str, "false") || !strcmp(argv[i].str, "off")) {
            argv[i].b = false;
        } else {
            argv[i].b = (argv[i].u == 0) ? false : true;
        }
    }
}


static status_t command_loop(int (*get_line)(const char **, void *), void *get_line_cookie, bool showprompt, bool locked)
{
    bool exit;
#if WITH_LIB_ENV
    bool report_result;
#endif
    cmd_args *args = NULL;
    const char *buffer;
    const char *continuebuffer;
    char *outbuf = NULL;

    args = (cmd_args *) malloc (MAX_NUM_ARGS * sizeof(cmd_args));
    if (unlikely(args == NULL)) {
        goto no_mem_error;
    }

    const size_t outbuflen = 1024;
    outbuf = malloc(outbuflen);
    if (unlikely(outbuf == NULL)) {
        goto no_mem_error;
    }

    exit = false;
    continuebuffer = NULL;
    while (!exit) {
        // read a new line if it hadn't been split previously and passed back from tokenize_command
        if (continuebuffer == NULL) {
            if (showprompt)
                fputs("] ", stdout);

            int len = get_line(&buffer, get_line_cookie);
            if (len < 0)
                break;
            if (len == 0)
                continue;
        } else {
            buffer = continuebuffer;
        }

//      dprintf("line = '%s'\n", buffer);

        /* tokenize the line */
        int argc = tokenize_command(buffer, &continuebuffer, outbuf, outbuflen,
                                    args, MAX_NUM_ARGS);
        if (argc < 0) {
            if (showprompt)
                printf("syntax error\n");
            continue;
        } else if (argc == 0) {
            continue;
        }

//      dprintf("after tokenize: argc %d\n", argc);
//      for (int i = 0; i < argc; i++)
//          dprintf("%d: '%s'\n", i, args[i].str);

        /* convert the args */
        convert_args(argc, args);

        /* try to match the command */
        const cmd *command = match_command(args[0].str, CMD_AVAIL_NORMAL);
        if (!command) {
            if (showprompt)
                printf("command not found\n");
            continue;
        }

        if (!locked)
            mutex_acquire(command_lock);

        abort_script = false;
        lastresult = command->cmd_callback(argc, args);

#if WITH_LIB_ENV
        bool report_result;
        env_get_bool("reportresult", &report_result, false);
        if (report_result) {
            if (lastresult < 0)
                printf("FAIL %d\n", lastresult);
            else
                printf("PASS %d\n", lastresult);
        }
#endif

#if WITH_LIB_ENV
        // stuff the result in an environment var
        env_set_int("?", lastresult, true);
#endif

        // someone must have aborted the current script
        if (abort_script)
            exit = true;
        abort_script = false;

        if (!locked)
            mutex_release(command_lock);
    }

    free(outbuf);
    free(args);
    return NO_ERROR;

no_mem_error:
    if (outbuf)
        free(outbuf);

    if (args)
        free(args);

    dprintf(INFO, "%s: not enough memory\n", __func__);
    return ERR_NO_MEMORY;
}

void console_abort_script(void)
{
    abort_script = true;
}

void console_start(void)
{
    debug_buffer = malloc(LINE_LEN);

    dprintf(INFO, "entering main console loop\n");


    while (command_loop(&read_debug_line, NULL, true, false) == NO_ERROR)
        ;

    dprintf(INFO, "exiting main console loop\n");

    free (debug_buffer);
}

struct line_read_struct {
    const char *string;
    int pos;
    char *buffer;
    size_t buflen;
};

static int fetch_next_line(const char **buffer, void *cookie)
{
    struct line_read_struct *lineread = (struct line_read_struct *)cookie;

    // we're done
    if (lineread->string[lineread->pos] == 0)
        return -1;

    size_t bufpos = 0;
    while (lineread->string[lineread->pos] != 0) {
        if (lineread->string[lineread->pos] == '\n') {
            lineread->pos++;
            break;
        }
        if (bufpos == (lineread->buflen - 1))
            break;
        lineread->buffer[bufpos] = lineread->string[lineread->pos];
        lineread->pos++;
        bufpos++;
    }
    lineread->buffer[bufpos] = 0;

    *buffer = lineread->buffer;

    return bufpos;
}

static int console_run_script_etc(const char *string, bool locked)
{
    struct line_read_struct lineread;

    lineread.string = string;
    lineread.pos = 0;
    lineread.buffer = malloc(LINE_LEN);
    lineread.buflen = LINE_LEN;

    command_loop(&fetch_next_line, (void *)&lineread, false, locked);

    free(lineread.buffer);

    return lastresult;
}

int console_run_script(const char *string)
{
    return console_run_script_etc(string, false);
}

int console_run_script_locked(const char *string)
{
    return console_run_script_etc(string, true);
}

console_cmd console_get_command_handler(const char *commandstr)
{
    const cmd *command = match_command(commandstr, CMD_AVAIL_NORMAL);

    if (command)
        return command->cmd_callback;
    else
        return NULL;
}

void console_register_commands(cmd_block *block)
{
    DEBUG_ASSERT(block);
    DEBUG_ASSERT(block->next == NULL);

    block->next = command_list;
    command_list = block;
}


static int cmd_help_impl(uint8_t availability_mask)
{
    printf("command list:\n");

    cmd_block *block;
    size_t i;

    for (block = command_list; block != NULL; block = block->next) {
        const cmd *curr_cmd = block->list;
        for (i = 0; i < block->count; i++) {
            if ((availability_mask & curr_cmd[i].availability_mask) == 0) {
                // Skip commands that aren't available in the current shell.
                continue;
            }
            if (curr_cmd[i].help_str)
                printf("\t%-16s: %s\n", curr_cmd[i].cmd_str, curr_cmd[i].help_str);
        }
    }

    return 0;
}

static int cmd_help(int argc, const cmd_args *argv)
{
    return cmd_help_impl(CMD_AVAIL_NORMAL);
}

static int cmd_help_panic(int argc, const cmd_args *argv)
{
    return cmd_help_impl(CMD_AVAIL_PANIC);
}

static int cmd_echo(int argc, const cmd_args *argv)
{
    if (argc > 1)
        echo = argv[1].b;
    return NO_ERROR;
}

static void read_line_panic(char *buffer, const size_t len, FILE *panic_fd)
{
    size_t pos = 0;

    for (;;) {
        int c;
        if ((c = getc(panic_fd)) < 0) {
            continue;
        }

        switch (c) {
            case '\r':
            case '\n':
                fputc('\n', panic_fd);
                goto done;
            case 0x7f: // backspace or delete
            case 0x8:
                if (pos > 0) {
                    pos--;
                    fputs("\b \b", panic_fd); // wipe out a character
                }
                break;
            default:
                buffer[pos++] = c;
                fputc(c, panic_fd);
        }
        if (pos == (len - 1)) {
            fputs("\nerror: line too long\n", panic_fd);
            pos = 0;
            goto done;
        }
    }
done:
    buffer[pos] = 0;
}

void panic_shell_start(void)
{
    dprintf(INFO, "entering panic shell loop\n");
    char input_buffer[PANIC_LINE_LEN];
    cmd_args args[MAX_NUM_ARGS];

    // panic_fd allows us to do I/O using the polling drivers.
    // These drivers function even if interrupts are disabled.
    FILE *panic_fd = get_panic_fd();
    if (!panic_fd)
        return;

    for (;;) {
        fputs("! ", panic_fd);
        read_line_panic(input_buffer, PANIC_LINE_LEN, panic_fd);

        int argc;
        char *tok = strtok(input_buffer, WHITESPACE);
        for (argc = 0; argc < MAX_NUM_ARGS; argc++) {
            if (tok == NULL) {
                break;
            }
            args[argc].str = tok;
            tok = strtok(NULL, WHITESPACE);
        }

        if (argc == 0) {
            continue;
        }

        convert_args(argc, args);

        const cmd *command = match_command(args[0].str, CMD_AVAIL_PANIC);
        if (!command) {
            fputs("command not found\n", panic_fd);
            continue;
        }

        command->cmd_callback(argc, args);
    }
}

#if LK_DEBUGLEVEL > 1
static int cmd_test(int argc, const cmd_args *argv)
{
    int i;

    printf("argc %d, argv %p\n", argc, argv);
    for (i = 0; i < argc; i++)
        printf("\t%d: str '%s', i %ld, u %#lx, b %d\n", i, argv[i].str, argv[i].i, argv[i].u, argv[i].b);

    return 0;
}
#endif
