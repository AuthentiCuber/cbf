#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BFCHARS "><+-[],."

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
    size_t length;
    TokenType list[];
} Tokens;

typedef struct {
    TokenType tokType;
    size_t param;
} Command;

typedef struct {
    size_t length;
    Command list[];
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
    for (short i = 0; i < 8; i++) {
        if (c == BFCHARS[i]) {
            return 1;
        }
    }
    return 0;
}

Tokens *tokenise(char *input, size_t inpLen) {
    Tokens *toks = malloc(sizeof(Tokens) + sizeof(TokenType) * inpLen);
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
    Commands *cmds = malloc(sizeof(Commands) + sizeof(Command) * toks->length);
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
    size_t jumpIndexStack[cmds->length * sizeof(size_t)];
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

    return cmds;
}

char *run(Commands *cmds) {
    char memory[30000] = {0};
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

    output[outputHead] = 0;
    return output;
}

int readFile(char *filePath, char **out) {
    FILE *file = fopen(filePath, "r");

    if (file == NULL) {
        return errno;
    }
    if (fseek(file, 0, SEEK_END) < 0) {
        return errno;
    }

    unsigned long fileLength = (unsigned long)ftell(file);

    if (fileLength <= 0) {
        return errno;
    }
    if (fseek(file, 0, SEEK_SET) < 0) {
        return errno;
    }

    char *data = malloc(fileLength);

    if (data == NULL) {
        return errno;
    }
    if (fread(data, 1, fileLength, file) < fileLength) {
        return ferror(file);
    }
    if (data == NULL) {
        perror("filling input buffer failed");
        return 1;
    }
    // null terminate
    data[fileLength] = 0;

    if (fclose(file) == EOF) {
        return errno;
    }

    *out = data;

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "please provide an input!\n");
        return EXIT_FAILURE;
    }

    char *inp;
    if (strcmp(argv[1], "--literal") == 0 || strcmp(argv[1], "-l") == 0) {
        inp = argv[2];
    } else {
        int readErr = readFile(argv[1], &inp);
        if (readErr != 0) {
            fprintf(stderr, "failed to read file %s: %s\n", argv[1],
                    strerror(readErr));
            return EXIT_FAILURE;
        }
    }

    Tokens *toks = tokenise(inp, strlen(inp));
    Commands *cmds = parse(toks);
    const char *output = run(cmds);
    printf("%s", output);

    return EXIT_SUCCESS;
}
