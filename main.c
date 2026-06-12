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

typedef struct {
    Command *list;
    size_t length;
} Commands;

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

Commands *parse(TokenType *toks, size_t numToks) {
    Commands *cmds = malloc(sizeof(Commands));
    cmds->list = calloc(numToks, sizeof(Command));
    size_t cmdIndex = 0;
    size_t paramCounter = 1;
    for (size_t i = 0; i < numToks; i++) {
        TokenType tok = toks[i];
        if (tok == JZ || tok == JNZ) {
            cmds->list[cmdIndex++] = (Command){tok, 0};
            paramCounter = 1;
        } else if (i + 1 < numToks && toks[i + 1] == tok) {
            paramCounter++;
        } else {
            cmds->list[cmdIndex++] = (Command){tok, paramCounter};
            paramCounter = 1;
        }
    }

    cmds->length = cmdIndex;
    return cmds;
}

int main(int argc, char **argv) {
    size_t inpLen = 24;
    char *inp = "++++++++[>+++++++++<-]>.";
    TokenType *toks = tokenise(inp, inpLen);
    Commands *cmds = parse(toks, inpLen);
    for (size_t i = 0; i < cmds->length; i++) {
        printf("%d: %zu\n", cmds->list[i].tokType, cmds->list[i].param);
        // printf("%d\n", toks[i]);
    }

    return EXIT_SUCCESS;
}
