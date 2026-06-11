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
    int param;
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
    }
}

void tokenise(char* input, int inpLen, TokenType *resultBuf) {
    for (int i = 0; i < inpLen; i++) {
        resultBuf[i] = makeToken(input[i]);
    }
}

int main(int argc, char **argv) {
    int inpLen = 8;
    TokenType *tokBuf = malloc(inpLen * sizeof(TokenType));
    char *inp = "><+-[],.";
    tokenise(inp, inpLen, tokBuf);

    for (int i = 0; i < inpLen; i++) {
        int tok = tokBuf[i];
        printf("%d\n", tok);
    }
    return 0;
}
