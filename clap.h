#ifndef CLAP_H_
#define CLAP_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLAP_MAKE_ARRAY(Type, name) \
    typedef struct {                \
        Type *items;                \
        size_t count;               \
    } name

typedef struct {
    char *name;
    int num_params;
    char *help;
} clap_arg;

CLAP_MAKE_ARRAY(clap_arg, clap_arg_array);

typedef struct {
    char *name;
    char **params;
    int num_params;
} clap_parsed;

CLAP_MAKE_ARRAY(clap_parsed, clap_parsed_array);

int clap_parse_args(clap_arg_array *to_parse, int argc, char **argv,
                    clap_parsed_array *options);

clap_parsed *clap_get_opt(clap_parsed_array *options, const char *opt_name);

bool clap_has_flag(clap_parsed_array *options, const char *flag_name);

void clap_show_help(clap_arg_array *expected, const char *prog_name,
                    const char *usage);

#endif // CLAP_H_

#ifdef CLAP_IMPLEMENTATION

int clap_parse_args(clap_arg_array *to_parse, int argc, char **argv,
                    clap_parsed_array *options) {
    options->items = calloc(to_parse->count, sizeof(clap_parsed));
    for (int argv_idx = 1; argv_idx < argc; argv_idx++) {
        char *inp = argv[argv_idx];
        for (size_t expected_arg_idx = 0; expected_arg_idx < to_parse->count;
             expected_arg_idx++) {
            clap_arg curr_arg = to_parse->items[expected_arg_idx];
            if (strcmp(inp, curr_arg.name) == 0) {
                char **params = calloc(curr_arg.num_params, sizeof(char *));
                int param_head_idx = 0;
                for (int i = 0; i < curr_arg.num_params; i++) {
                    if (argv_idx + 1 >= argc) return 1;
                    params[param_head_idx++] = argv[++argv_idx];
                }
                options->items[options->count++] =
                    (clap_parsed){curr_arg.name, params, curr_arg.num_params};
            }
        }
    }
    return 0;
}

clap_parsed *clap_get_opt(clap_parsed_array *options, const char *opt_name) {
    for (size_t i = 0; i < options->count; i++) {
        clap_parsed *opt = &options->items[i];
        if (strcmp(opt->name, opt_name) == 0) return opt;
    }
    return NULL;
}

bool clap_has_flag(clap_parsed_array *options, const char *flag_name) {
    return clap_get_opt(options, flag_name) != NULL;
}

void clap_show_help(clap_arg_array *expected, const char *prog_name,
                    const char *usage) {
    printf("Usage: %s ", prog_name);
    if (usage == NULL) {
        printf("[OPTIONS]");
    } else {
        printf("%s", usage);
    }
    printf("\n\nAvailable options:\n");

    const char *arg_str = "<arg>";
    int longest_len = 0;
    for (size_t i = 0; i < expected->count; i++) {
        clap_arg opt = expected->items[i];
        int curr_len =
            strlen(opt.name) + opt.num_params * (1 + strlen(arg_str));
        if (curr_len > longest_len) { longest_len = curr_len; }
    }
    for (size_t i = 0; i < expected->count; i++) {
        clap_arg opt = expected->items[i];
        printf("  ");
        int line_len = printf("%s", opt.name);
        for (int j = 0; j < opt.num_params; j++) {
            line_len += printf(" %s", arg_str);
        }
        while (line_len < longest_len) {
            putchar(' ');
            line_len++;
        }
        printf("  %s\n", opt.help);
    }
}
#endif // CLAP_IMPLEMENTATION
