#define CLAP_IMPLEMENTATION
#include "clap.h"
#include "libcbf.h"
#include <errno.h>

void do_repl(size_t bf_mem_size, FILE *in_stream, FILE *out_stream) {
    fprintf(out_stream,
            "cbf: a simple interactive brainfuck interpreter\n"
            "(memory tape %zu x 1 byte cells)\n"
            "Type `exit` or CTRL-D to exit\n",
            bf_mem_size);

    memory bf_mem = {calloc(bf_mem_size, 1), bf_mem_size, 0};

    char line[200];
    for (;;) {
        fprintf(out_stream, "bf> ");

        fflush(out_stream);
        const char *line_err = fgets(line, sizeof(line), in_stream);
        if (line_err == NULL || strcmp(line, "exit\n") == 0) {
            if (feof(in_stream)) { putc('\n', out_stream); }
            break;
        }

        int num_chars_printed = run_bf(strlen(line), line, &bf_mem, 0,
                                       in_stream, out_stream, out_stream);

        if (num_chars_printed != 0) { putc('\n', out_stream); }
    }

    free(bf_mem.cells);
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

#define HELP_TEXT                                                           \
    "Usage:\n"                                                              \
    "  %s [OPTIONS] run [-d] <file>  Run a file containing bf code\n"       \
    "  %s [OPTIONS] repl             Run bf code interactively in a repl\n" \
    "\n"                                                                    \
    "Glabal options:\n"                                                     \
    "  --help, -h     Print this help message\n"                            \
    "  --memsize, -m  Set the maximum bf memory tape size\n"                \
    "\n"                                                                    \
    "Mode specific options:\n"                                              \
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

    size_t bf_mem_size = 30000;

    if ((opt = clap_get_opt(parsed, "--memsize")) != NULL) {
        bf_mem_size = strtoul(opt->params[0], NULL, 10);
    }

    if (clap_has_flag(parsed, "repl")) {
        do_repl(bf_mem_size, stdin, stdout);
        return EXIT_SUCCESS;
    }

    bool debug = clap_has_flag(parsed, "--debug");

    if ((opt = clap_get_opt(parsed, "run")) != NULL) {
        if (argc < 3) {
            fprintf(stderr, "Please provide a file!\n\n");
            show_help(prog_name);
            return EXIT_FAILURE;
        }

        const char *file_name = opt->params[0];
        char *inp;
        int read_err = read_file(file_name, &inp);
        if (read_err != 0) {
            fprintf(stderr, "failed to read file %s: %s\n", file_name,
                    strerror(read_err));
            return EXIT_FAILURE;
        }

        memory bf_mem = {calloc(bf_mem_size, 1), bf_mem_size, 0};
        size_t inp_len = strlen(inp);

        int num_chars_printed =
            run_bf(inp_len, inp, &bf_mem, debug, stdin, stdout, stderr);
        if (num_chars_printed < 0) return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }

    fprintf(stderr, "Provided arguments not recognised!\n\n");
    show_help(prog_name);
    return EXIT_FAILURE;
}
