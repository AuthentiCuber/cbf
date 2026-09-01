#include "libcbf.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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

int tokenise(const char *input, size_t inp_len, tok_type_arr *toks_out) {
    for (size_t i = 0; i < inp_len; i++) {
        int tok_type = make_token(input[i]);
        if (tok_type >= 0) {
            toks_out->items[toks_out->count++] = (token_type)tok_type;
        }
    }
    return 0;
}

void invert_tok(token_type *tt) {
    switch (*tt) {
    case DATA_INC: *tt = DATA_DEC; break;
    case DATA_DEC: *tt = DATA_INC; break;
    case DP_INC:   *tt = DP_DEC; break;
    case DP_DEC:   *tt = DP_INC; break;
    default:       break;
    }
}

collapse_result can_collapse_toks(token_type a, token_type b) {
    switch (a) {
    case JZ: // FALLTHROUGH
    case JNZ: return COLLAPSE_NONE;
    case DP_INC:
        if (b == DP_DEC) { return COLLAPSE_CANCEL; }
        break;
    case DP_DEC:
        if (b == DP_INC) { return COLLAPSE_CANCEL; }
        break;
    case DATA_INC:
        if (b == DATA_DEC) { return COLLAPSE_CANCEL; }
        break;
    case DATA_DEC:
        if (b == DATA_INC) { return COLLAPSE_CANCEL; }
        break;
    default: break;
    }
    return a == b ? COLLAPSE_ADD : COLLAPSE_NONE;
}

void collapse_repeated_toks(tok_type_arr *tok_types_in, tok_arr *toks_out) {
    int param_counter = 1;
    size_t tok_type_scanner = 0;
    while (tok_type_scanner < tok_types_in->count) {
        token_type curr_tok_type = tok_types_in->items[tok_type_scanner];

        bool curr_tok_collapsed = false;
        while (!curr_tok_collapsed) {
            if (++tok_type_scanner >= tok_types_in->count) { break; }

            collapse_result can_collapse = can_collapse_toks(
                curr_tok_type, tok_types_in->items[tok_type_scanner]);

            switch (can_collapse) {
            case COLLAPSE_NONE:   curr_tok_collapsed = true; break;
            case COLLAPSE_ADD:    param_counter += 1; break;
            case COLLAPSE_CANCEL: param_counter -= 1; break;
            }
        }
        if (param_counter < 0) {
            param_counter *= -1;
            invert_tok(&curr_tok_type);
        }
        toks_out->items[toks_out->count++] =
            (token){curr_tok_type, {.numtimes = (size_t)param_counter}};
        param_counter = 1;
    }
}

parse_result resolve_jump_locs(tok_arr *toks) {
    size_t jump_idx_stack[toks->count * sizeof(int)];
    size_t jump_idx_stack_head = 0;
    for (size_t tok_idx = 0; tok_idx < toks->count; tok_idx++) {
        switch (toks->items[tok_idx].type) {
        case JZ: jump_idx_stack[jump_idx_stack_head++] = tok_idx; break;
        case JNZ:
            // trying to pop from empty stack
            if (jump_idx_stack_head == 0) {
                return (parse_result){UNBALANCED_RIGHT, tok_idx};
            }

            size_t jump_pos = jump_idx_stack[--jump_idx_stack_head];
            toks->items[tok_idx].jumploc = jump_pos;
            toks->items[jump_pos].jumploc = tok_idx;
            break;
        default: break;
        }
    }
    // leftover items in stack
    if (jump_idx_stack_head != 0) {
        return (parse_result){UNBALANCED_LEFT, jump_idx_stack[0]};
    }
    return (parse_result){PARSE_SUCCESS, 0};
}

parse_result parse(tok_type_arr *tok_types_in, tok_arr *toks_out) {
    collapse_repeated_toks(tok_types_in, toks_out);

    return resolve_jump_locs(toks_out);
}

run_result interpret_cmds(memory *mem, tok_arr *toks) {
    size_t cmd_ptr = 0;
    size_t num_printed = 0;
    while (cmd_ptr < toks->count) {
        token curr_tok = toks->items[cmd_ptr];
        switch (curr_tok.type) {
        case DP_INC:
            mem->dataPtr += curr_tok.numtimes;
            if (mem->dataPtr > mem->length) {
                return (run_result){DATA_PTR_OOB, cmd_ptr, num_printed};
            }
            break;
        case DP_DEC:
            // unsigned ints, cannot check for < 0 after decrement
            if (mem->dataPtr < curr_tok.numtimes) {
                return (run_result){DATA_PTR_OOB, cmd_ptr, num_printed};
            }
            mem->dataPtr -= curr_tok.numtimes;
            break;
        case DATA_INC: mem->cells[mem->dataPtr] += curr_tok.numtimes; break;
        case DATA_DEC: mem->cells[mem->dataPtr] -= curr_tok.numtimes; break;
        case INPUT:    mem->cells[mem->dataPtr] = (unsigned char)getchar(); break;
        case OUTPUT:
            for (size_t i = 0; i < curr_tok.numtimes; i++) {
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

    return (run_result){RUN_SUCCESS, 0, num_printed};
}

int run_bf(size_t inp_len, const char *inp, memory *mem, bool debug) {
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

    parse_result parse_err = parse(&toks, &cmds);

    if (parse_err.status == UNBALANCED_RIGHT) {
        fprintf(stderr,
                "Aborted: Unbalanced closing bracket found at pos %zu\n",
                parse_err.error_loc);
        free(toks.items);
        free(cmds.items);
        return -1;
    } else if (parse_err.status == UNBALANCED_LEFT) {
        fprintf(stderr,
                "Aborted: Unbalanced opening bracket found at pos %zu\n",
                parse_err.error_loc);
        free(toks.items);
        free(cmds.items);
        return -1;
    }

    if (debug) {
        printf("------- Generated commands start -------\n");
        for (size_t i = 0; i < cmds.count; i++) {
            // numtimes isnt technically correct here...
            printf("%d: %zu\n", cmds.items[i].type, cmds.items[i].numtimes);
        }
        printf("------- Generated commands end -------\n");
        printf("------- Output start -------\n");
    }

    run_result run_err = interpret_cmds(mem, &cmds);

    if (debug) { printf("\n------- Output end -------\n"); }

    if (run_err.status == DATA_PTR_OOB) {
        fprintf(stderr,
                "Data pointer out of bounds! (from instruction at pos %zu)\n",
                run_err.error_loc);
        free(toks.items);
        free(cmds.items);
        return -1;
    }

    return run_err.chars_printed;
}
