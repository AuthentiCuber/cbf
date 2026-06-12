#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    DP_INC,
    DP_DEC,
    DATA_INC,
    DATA_DEC,
    INPUT,
    OUTPUT,
    JZ,
    JNZ
} TokenType;

typedef struct {
    TokenType tokType;
    size_t param;
} Command;

TokenType makeToken(char c) {
    switch (c) {
    case '>':
        return DP_INC;
        break;
    case '<':
        return DP_DEC;
        break;
    case '+':
        return DATA_INC;
        break;
    case '-':
        return DATA_DEC;
        break;
    case '[':
        return JZ;
        break;
    case ']':
        return JNZ;
        break;
    case '.':
        return OUTPUT;
        break;
    case ',':
        return INPUT;
        break;
    default:
        exit(EXIT_FAILURE);
    }
}

TokenType *tokenise(char *input, size_t inpLen) {
    TokenType *tokBuf = calloc(inpLen, sizeof(TokenType));
    for (size_t i = 0; i < inpLen; i++) {
        tokBuf[i] = makeToken(input[i]);
    }
    return tokBuf;
}

Command *parse(TokenType *toks, size_t numToks) {
    Command *cmds = calloc(numToks, sizeof(Command));
    size_t cmdIndex = 0;
    size_t paramCounter = 0;
    for (size_t i = 0; i < numToks; i++) {
        TokenType tok = toks[i];
        if (tok == JZ || tok == JNZ) {
            cmds[cmdIndex] = (Command){tok, 0};
            cmdIndex++;
            paramCounter = 0;
        } else {
            if (toks[i + 1] == tok) {
                paramCounter++;
            } else {
                cmds[cmdIndex] = (Command){tok, paramCounter};
                cmdIndex++;
                paramCounter = 0;
            }
        }
    }
    return cmds;
}

int main(int argc, char **argv) {
    size_t inpLen = 8;
    char *inp = "><+-[],.";
    TokenType *toks = tokenise(inp, inpLen);

    for (size_t i = 0; i < inpLen; i++) {
        int tok = toks[i];
        printf("%d\n", tok);
    }
    return EXIT_SUCCESS;
}
