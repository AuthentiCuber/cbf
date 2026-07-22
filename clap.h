/*
    clap 1.1.0 - Public Domain - https://github.com/AuthentiCuber/clap
    A basic command line argument parser
*/

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
    char *alias;
    int num_params;
    char *help;
} clap_arg;

CLAP_MAKE_ARRAY(clap_arg, clap_arg_array);

typedef struct {
    char *name;
    char *alias;
    char **params;
    int num_params;
} clap_parsed;

CLAP_MAKE_ARRAY(clap_parsed, clap_parsed_array);

CLAP_MAKE_ARRAY(char *, clap_unexpected_array);

// Parses argv according to to_parse, storing the results in parsed.
int clap_parse_args(clap_arg_array *to_parse, int argc, char **argv,
                    clap_parsed_array *options,
                    clap_unexpected_array *unexpected);

// Searches in options for opt_name and returns a pointer to it if found, or
// NULL otherwise.
clap_parsed *clap_get_opt(clap_parsed_array *options, const char *opt_name);

// Checks whether flag_name was passed or not.
bool clap_has_flag(clap_parsed_array *options, const char *flag_name);

// Prints usage/help text, generated from the expected arguments. You can
// provide your own usage string, or leave it NULL for a generic one.
void clap_show_help(clap_arg_array *expected, const char *prog_name,
                    const char *usage);

#endif // CLAP_H_

#ifdef CLAP_IMPLEMENTATION

int clap_parse_args(clap_arg_array *to_parse, int argc, char **argv,
                    clap_parsed_array *options,
                    clap_unexpected_array *unexpected) {
    if (options == NULL || to_parse == NULL || unexpected == NULL) return 2;

    options->count = 0;
    options->items = calloc(to_parse->count, sizeof(clap_parsed));
    if (options->items == NULL) return 2;

    unexpected->count = 0;
    unexpected->items = calloc(100, sizeof(char *));
    if (unexpected->items == NULL) return 2;

    for (int argv_idx = 1; argv_idx < argc; argv_idx++) {
        char *curr_passed_arg = argv[argv_idx];
        bool expected = false;
        for (size_t expected_arg_idx = 0; expected_arg_idx < to_parse->count;
             expected_arg_idx++) {
            clap_arg curr_expected_arg = to_parse->items[expected_arg_idx];
            if (strcmp(curr_passed_arg, curr_expected_arg.name) == 0 ||
                strcmp(curr_passed_arg, curr_expected_arg.alias) == 0) {
                expected = true;
                char **params =
                    calloc(curr_expected_arg.num_params, sizeof(char *));
                if (params == NULL) return 2;

                int param_head_idx = 0;
                for (int i = 0; i < curr_expected_arg.num_params; i++) {
                    if (argv_idx + 1 >= argc) {
                        free(params);
                        return 1;
                    }
                    params[param_head_idx++] = argv[++argv_idx];
                }
                options->items[options->count++] = (clap_parsed){
                    curr_expected_arg.name, curr_expected_arg.alias, params,
                    curr_expected_arg.num_params};
            }
        }
        if (!expected) {
            unexpected->items[unexpected->count++] = curr_passed_arg;
        }
    }
    return 0;
}

clap_parsed *clap_get_opt(clap_parsed_array *options, const char *opt_name) {
    for (size_t i = 0; i < options->count; i++) {
        clap_parsed *opt = &options->items[i];
        if (strcmp(opt->name, opt_name) == 0 ||
            strcmp(opt->alias, opt_name) == 0)
            return opt;
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

    // this gets printed for each argument an option takes
    const char *arg_str = "<arg>";
    // find the longest resulting option (for alignment)
    int longest_len = 0;
    for (size_t i = 0; i < expected->count; i++) {
        clap_arg opt = expected->items[i];
        int curr_len = strlen(opt.alias) + 2 + strlen(opt.name) +
                       opt.num_params * (1 + strlen(arg_str));
        if (curr_len > longest_len) { longest_len = curr_len; }
    }
    for (size_t i = 0; i < expected->count; i++) {
        clap_arg opt = expected->items[i];
        printf("  ");
        int line_len = 0;
        if (*opt.alias != '\0') { line_len += printf("%s, ", opt.alias); }
        line_len += printf("%s", opt.name);
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
