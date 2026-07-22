#define CLAP_IMPLEMENTATION
#include "clap.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKE_ARRAY_TYPE(Type, name) \
    typedef struct {                \
        Type *items;                \
        size_t count;               \
    } name

typedef enum {
    DP_INC,
    DP_DEC,
    DATA_INC,
    DATA_DEC,
    INPUT,
    OUTPUT,
    JZ,
    JNZ
} token_type;

MAKE_ARRAY_TYPE(token_type, tok_type_arr);

typedef struct {
    token_type type;
    union {
        size_t jumploc; // for jz, jnz
        int numtimes;   // otherwise
    };
} token;

MAKE_ARRAY_TYPE(token, tok_arr);

typedef struct {
    unsigned char *cells;
    int length;
    int dataPtr;
} memory;

int make_token(const char c) {
    switch (c) {
    case '>': return DP_INC;
    case '<': return DP_DEC;
    case '+': return DATA_INC;
    case '-': return DATA_DEC;
    case '[': return JZ;
    case ']': return JNZ;
    case '.': return OUTPUT;
    case ',': return INPUT;
    default:  return -1;
    }
}

int tokenise(const char *input, const size_t inp_len, tok_type_arr *tok_types) {
    for (size_t i = 0; i < inp_len; i++) {
        int tok_type = make_token(input[i]);
        if (tok_type >= 0) {
            tok_types->items[tok_types->count++] = (token_type)tok_type;
        }
    }
    return 0;
}

int can_collapse(token_type a, token_type b) {
    switch (a) {
    case JZ: // FALLTHROUGH
    case JNZ: return 0;
    case DP_INC:
        if (b == DP_DEC) { return -1; }
        break;
    case DP_DEC:
        if (b == DP_INC) { return -1; }
        break;
    case DATA_INC:
        if (b == DATA_DEC) { return -1; }
        break;
    case DATA_DEC:
        if (b == DATA_INC) { return -1; }
        break;
    default: break;
    }
    return a == b;
}

/* Parses TOKS into CMDS, combining collapsable tokens.
Returns a negative value if an unbalanced closing
bracket is found, or a positive value if an
unbalanced opening bracket is found. */
int parse(tok_type_arr *tok_types, tok_arr *toks) {
    // combine collapsable tokens
    int param_counter = 1;
    size_t tok_type_scanner = 0;
    while (tok_type_scanner < tok_types->count) {
        token_type curr_tok_type = tok_types->items[tok_type_scanner];
        for (;;) {
            if (tok_type_scanner++ >= tok_types->count) { break; }

            int collapse_sign =
                can_collapse(curr_tok_type, tok_types->items[tok_type_scanner]);
            if (collapse_sign == 0) { break; }

            param_counter += collapse_sign;
        }
        toks->items[toks->count++] =
            (token){curr_tok_type, {.numtimes = param_counter}};
        param_counter = 1;
    }

    // jump location resolution
    size_t jump_idx_stack[toks->count * sizeof(int)];
    size_t jump_idx_stack_head = 0;
    for (size_t i = 0; i < toks->count; i++) {
        switch (toks->items[i].type) {
        case JZ: jump_idx_stack[jump_idx_stack_head++] = i; break;
        case JNZ:
            // trying to pop from empty stack
            if (jump_idx_stack_head == 0) { return -1 - (int)i; }

            size_t jump_pos = jump_idx_stack[--jump_idx_stack_head];
            toks->items[i].jumploc = jump_pos;
            toks->items[jump_pos].jumploc = i;
            break;
        default: break;
        }
    }
    // leftover items in stack
    if (jump_idx_stack_head != 0) {
        return (int)jump_idx_stack[--jump_idx_stack_head] + 1;
    }
    return 0;
}

/* Execute CMDS given MEMORY of lengith MEMSIZE and the current
DATAPTR position. On success returns the number of characters printed,
a negative return means that DATAPTR went out of bounds. */
int interpret_cmds(memory *mem, tok_arr *toks) {
    size_t cmd_ptr = 0;
    int num_printed = 0;
    while (cmd_ptr < toks->count) {
        token curr_tok = toks->items[cmd_ptr];
        switch (curr_tok.type) {
        case DP_INC:
            mem->dataPtr += curr_tok.numtimes;
            if (mem->dataPtr > mem->length) { return -2; }
            break;
        case DP_DEC:
            // unsigned ints, cannot check for < 0 after decrement
            if (mem->dataPtr < curr_tok.numtimes) { return -1; }
            mem->dataPtr -= curr_tok.numtimes;
            break;
        case DATA_INC: mem->cells[mem->dataPtr] += curr_tok.numtimes; break;
        case DATA_DEC: mem->cells[mem->dataPtr] -= curr_tok.numtimes; break;
        case INPUT:    mem->cells[mem->dataPtr] = (unsigned char)getchar(); break;
        case OUTPUT:
            for (int i = 0; i < curr_tok.numtimes; i++) {
                putchar(mem->cells[mem->dataPtr]);
                num_printed++;
            }
            break;
        case JZ:
            if (mem->cells[mem->dataPtr] == 0) { cmd_ptr = curr_tok.jumploc; }
            break;
        case JNZ:
            if (mem->cells[mem->dataPtr] != 0) { cmd_ptr = curr_tok.jumploc; }
            break;
        }
        cmd_ptr++;
    }

    return num_printed;
}

/* Runs INP as bf code, given MEMORY, DATAPTR, etc. Like interpretCmds,
 returns number of characters printed and a negative value on error. */
int run_bf(size_t inp_len, const char *inp, memory *mem, int debug) {
    tok_type_arr toks = {calloc(inp_len, sizeof(token_type)), 0};

    tokenise(inp, inp_len, &toks);
    if (debug) {
        printf("------- Generated tokens start -------\n");
        for (size_t i = 0; i < toks.count; i++) {
            printf("%d ", toks.items[i]);
        }
        printf("\n------- Generated tokens end -------\n");
    }

    tok_arr cmds = {calloc(toks.count, sizeof(token)), 0};

    int parse_err = parse(&toks, &cmds);

    if (parse_err < 0) {
        fprintf(stderr, "Unbalanced closing bracket found at position %d\n",
                abs(parse_err + 1));
        return -1;
    } else if (parse_err > 0) {
        fprintf(stderr, "Unbalanced opening bracket found at position %d\n",
                parse_err - 1);
        return -1;
    }

    if (debug) {
        printf("------- Generated commands start -------\n");
        for (size_t i = 0; i < cmds.count; i++) {
            // numtimes isnt technically correct here...
            printf("%d: %d\n", cmds.items[i].type, cmds.items[i].numtimes);
        }
        printf("------- Generated commands end -------\n");
        printf("------- Output start -------\n");
    }

    int chars_printed = interpret_cmds(mem, &cmds);

    if (debug) { printf("\n------- Output end -------\n"); }

    if (chars_printed < 0) {
        fprintf(stderr, "Data pointer out of bounds!\n");
        return -1;
    }

    return chars_printed;
}

/* Helper function that reads the entirety of the
 file at FILEPATH into the string pointed to by OUT. */
int read_file(const char *file_path, char **out) {
    FILE *file = fopen(file_path, "r");

    if (file == NULL) { return errno; }
    if (fseek(file, 0, SEEK_END) < 0) { return errno; }

    unsigned long file_length = (unsigned long)ftell(file);

    if (file_length <= 0) { return errno; }
    if (fseek(file, 0, SEEK_SET) < 0) { return errno; }

    char *data = malloc(file_length);

    if (data == NULL) { return errno; }
    if (fread(data, 1, file_length, file) < file_length) {
        return ferror(file);
    }
    if (data == NULL) {
        perror("filling input buffer failed");
        return 1;
    }
    // null terminate
    data[file_length - 1] = 0;

    if (fclose(file) == EOF) { return errno; }

    *out = data;

    return 0;
}

#define HELP_TEXT                                                      \
    "Usage:\n"                                                         \
    "  %s [OPTIONS] run [-d] <file>  Run a file containing bf code\n"  \
    "  %s [OPTIONS] repl        Run bf code interactively in a repl\n" \
    "\n"                                                               \
    "Glabal options:\n"                                                \
    "  --help, -h     Print this help message\n"                       \
    "  --memsize, -m  Set the maximum bf memory tape size\n"           \
    "\n"                                                               \
    "Mode specific options:\n"                                         \
    "  --debug, -d     With `run`, show debug information\n"

void show_help(const char *prog_name) {
    fprintf(stderr, HELP_TEXT, prog_name, prog_name);
}

int main(int argc, char **argv) {
    char *prog_name = argv[0];
    clap_arg args[] = {{"--help", "-h", 0, NULL},
                       {"--memsize", "-m", 1, NULL},
                       {"repl", "", 0, NULL},
                       {"--debug", "-d", 0, NULL},
                       {"run", "", 1, NULL}};
    clap_arg_array args_arr = {args, sizeof(args) / sizeof(args[0])};
    clap_parsed_array *parsed = malloc(sizeof(clap_parsed_array));
    clap_unexpected_array *unexpected = malloc(sizeof(clap_unexpected_array));
    int parse_err = clap_parse_args(&args_arr, argc, argv, parsed, unexpected);
    if (parse_err != 0) {
        fprintf(stderr, "Failed to parse arguments!\n\n");
        show_help(prog_name);
        return EXIT_FAILURE;
    }

    if (clap_has_flag(parsed, "--help")) {
        show_help(prog_name);
        return EXIT_SUCCESS;
    }

    clap_parsed *opt;

    size_t bfMemSize = 30000;

    if ((opt = clap_get_opt(parsed, "--memsize")) != NULL) {
        bfMemSize = strtoul(opt->params[0], NULL, 10);
    }

    if (clap_has_flag(parsed, "repl")) {
        printf("cbf: a simple interactive brainfuck interpreter\n"
               "(memory tape %zu x 1 byte cells)\n"
               "Type `exit` or CTRL-D to exit\n",
               bfMemSize);

        memory bfmem = {calloc(bfMemSize, 1), (int)bfMemSize, 0};

        char line[200];
        for (;;) {
            printf("bf> ");

            fflush(stdout);
            const char *lineErr = fgets(line, sizeof(line), stdin);
            if (lineErr == NULL || strcmp(line, "exit\n") == 0) {
                if (feof(stdin)) { putchar('\n'); }
                break;
            }

            int charsPrinted = run_bf(strlen(line), line, &bfmem, 0);

            if (charsPrinted != 0) { putchar('\n'); }
        }
        return EXIT_SUCCESS;
    }

    int debug = 0;
    if (clap_has_flag(parsed, "--debug")) { debug = 1; }

    if ((opt = clap_get_opt(parsed, "run")) != NULL) {
        if (argc < 3) {
            fprintf(stderr, "Please provide a file!\n\n");
            show_help(prog_name);
            return EXIT_FAILURE;
        }

        const char *fileName = opt->params[0];
        char *inp;
        int readErr = read_file(fileName, &inp);
        if (readErr != 0) {
            fprintf(stderr, "failed to read file %s: %s\n", fileName,
                    strerror(readErr));
            return EXIT_FAILURE;
        }

        memory bfMem = {calloc(bfMemSize, 1), (int)bfMemSize, 0};
        size_t inpLen = strlen(inp);

        int numCharsPrinted = run_bf(inpLen, inp, &bfMem, debug);
        if (numCharsPrinted < 0) { return numCharsPrinted; }
        return 0;
    }

    fprintf(stderr, "Provided arguments not recognised!\n\n");
    show_help(prog_name);
    return EXIT_FAILURE;
}
