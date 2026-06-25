#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    TokenType *list;
    size_t length;
} Tokens;

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

int isValidBF(char c) {
    const char *bfChars = "><+-[],.";
    for (short i = 0; i < 8; i++) {
        if (c == bfChars[i]) {
            return 1;
        }
    }
    return 0;
}

Tokens *tokenise(char *input, size_t inpLen) {
    Tokens *toks = malloc(sizeof(Tokens));
    toks->list = calloc(inpLen, sizeof(TokenType));
    size_t tokBufHead = 0;
    for (size_t i = 0; i < inpLen; i++) {
        char c = input[i];
        if (!isValidBF(c)) {
            continue;
        }

        toks->list[tokBufHead++] = makeToken(c);
    }
    toks->length = tokBufHead;
    return toks;
}

Commands *parse(Tokens *toks) {
    Commands *cmds = malloc(sizeof(Commands));
    cmds->list = calloc(toks->length, sizeof(Command));
    size_t cmdIndex = 0;
    size_t paramCounter = 1;
    for (size_t i = 0; i < toks->length; i++) {
        TokenType tok = toks->list[i];
        if (tok == JZ || tok == JNZ) {
            cmds->list[cmdIndex++] = (Command){tok, 0};
            paramCounter = 1;
        } else if (i + 1 < toks->length && toks->list[i + 1] == tok) {
            paramCounter++;
        } else {
            cmds->list[cmdIndex++] = (Command){tok, paramCounter};
            paramCounter = 1;
        }
    }
    cmds->length = cmdIndex;

    // jump location resolution
    size_t *jumpIndexStack = calloc(cmds->length, sizeof(size_t));
    size_t jumpIndexStackHead = 0;
    for (size_t i = 0; i < cmds->length; i++) {
        switch (cmds->list[i].tokType) {
        case JZ:
            jumpIndexStack[jumpIndexStackHead++] = i;
            break;
        case JNZ:
            if (jumpIndexStackHead == 0) {
                fprintf(stderr, "One or more closing brackets are missing an "
                                "opening bracket!\n");
                exit(EXIT_FAILURE);
            }

            size_t jumpPos = jumpIndexStack[--jumpIndexStackHead];
            cmds->list[i].param = jumpPos;
            cmds->list[jumpPos].param = i;
            break;
        default:
            break;
        }
    }
    if (jumpIndexStackHead != 0) {
        fprintf(
            stderr,
            "One or more opening brackets are missing a closing bracket!\n");
        exit(EXIT_FAILURE);
    }
    free(jumpIndexStack);

    return cmds;
}

char *run(Commands *cmds) {
    char *memory = malloc(30000);
    int dataPtr = 0;
    size_t cmdPtr = 0;
    char *output = malloc(1000 * 1000);
    size_t outputHead = 0;
    while (cmdPtr < cmds->length) {
        const Command currCmd = cmds->list[cmdPtr];
        switch (currCmd.tokType) {
        case DP_INC:
            dataPtr += currCmd.param;
            break;
        case DP_DEC:
            dataPtr -= currCmd.param;
            break;
        case DATA_INC:
            memory[dataPtr] += currCmd.param;
            break;
        case DATA_DEC:
            memory[dataPtr] -= currCmd.param;
            break;
        case INPUT:
            memory[dataPtr] = (char)getchar();
            break;
        case OUTPUT:
            for (size_t i = 0; i < currCmd.param; i++) {
                output[outputHead++] = memory[dataPtr];
            }
            break;
        case JZ:
            if (memory[dataPtr] == 0) {
                cmdPtr = currCmd.param;
            }
            break;
        case JNZ:
            if (memory[dataPtr] != 0) {
                cmdPtr = currCmd.param;
            }
            break;
        }

        cmdPtr++;
    }
    free(memory);

    output[outputHead] = 0;
    return output;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "please provide an input file!\n");
        return EXIT_FAILURE;
    }

    FILE *inpFile = fopen(argv[1], "r");

    if (inpFile == NULL) {
        perror("failed to open file");
        return EXIT_FAILURE;
    }
    if (fseek(inpFile, 0, SEEK_END) < 0) {
        perror("failed to seek end");
        return EXIT_FAILURE;
    }

    long fileLength = ftell(inpFile);

    if (fileLength < 0) {
        perror("ftell failed");
        return EXIT_FAILURE;
    }
    if (fseek(inpFile, 0, SEEK_SET) < 0) {
        perror("faled to seek set");
        return EXIT_FAILURE;
    }

    char *inp = malloc(fileLength);

    if (inp == NULL) {
        perror("failed to allocate input buffer");
        return EXIT_FAILURE;
    }
    if (fread(inp, 1, fileLength, inpFile) < fileLength) {
        fprintf(stderr, "failed to read file: %s\n", strerror(ferror(inpFile)));
        return EXIT_FAILURE;
    }
    if (inp == NULL) {
        perror("filling input buffer failed");
        return EXIT_FAILURE;
    }
    // null terminate
    inp[fileLength] = 0;

    if (fclose(inpFile) == EOF) {
        perror("failed to close file");
        return EXIT_FAILURE;
    }

    Tokens *toks = tokenise(inp, strlen(inp));
    Commands *cmds = parse(toks);
    const char *output = run(cmds);
    printf("%s", output);

    return EXIT_SUCCESS;
}
