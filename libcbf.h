#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

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
        size_t jumploc;  // for jz, jnz
        size_t numtimes; // otherwise
    };
} token;

MAKE_ARRAY_TYPE(token, tok_arr);

#undef MAKE_ARRAY_TYPE

typedef struct {
    unsigned char *cells;
    size_t length;
    size_t dataPtr;
} memory;

typedef enum {
    COLLAPSE_CANCEL = -1,
    COLLAPSE_ADD = 1,
    COLLAPSE_NONE = 0,
} collapse_result;

typedef struct {
    enum { UNBALANCED_RIGHT, UNBALANCED_LEFT, PARSE_SUCCESS } status;
    size_t error_loc;
} parse_result;

typedef struct {
    enum { DATA_PTR_OOB, RUN_SUCCESS } status;
    size_t error_loc;
    size_t chars_printed;
} run_result;

int make_token(const char c);
void invert_tok(token_type *tt);
int tokenise(const char *input, size_t inp_len, tok_type_arr *toks_out);

collapse_result can_collapse_toks(token_type a, token_type b);
void collapse_repeated_toks(tok_type_arr *tok_types_in, tok_arr *toks_out);
parse_result resolve_jump_locs(tok_arr *toks);

parse_result parse(tok_type_arr *tok_types_in, tok_arr *toks_out);

run_result interpret_cmds(memory *mem, tok_arr *toks, FILE *in_stream,
                          FILE *out_stream);

int run_bf(size_t inp_len, const char *inp, memory *mem, bool debug);
