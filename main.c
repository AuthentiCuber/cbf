#include <errno.h>
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

#ifndef MEM_SIZE
#define MEM_SIZE 30000
#endif // MEM_SIZE

int makeToken(char c) {
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
        return -1;
    }
}

int tokenise(const char *input, size_t inpLen, Tokens *toks) {
    for (size_t i = 0; i < inpLen; i++) {
        int tok = makeToken(input[i]);
        if (tok >= 0) {
            toks->list[toks->length++] = (TokenType)tok;
        }
    }
    return 0;
}

int parse(Tokens *toks, Commands *cmds) {
    size_t paramCounter = 1;
    for (size_t i = 0; i < toks->length; i++) {
        TokenType tok = toks->list[i];
        if (tok == JZ || tok == JNZ) {
            cmds->list[cmds->length++] = (Command){tok, 0};
            paramCounter = 1;
        } else if (i + 1 < toks->length && toks->list[i + 1] == tok) {
            paramCounter++;
        } else {
            cmds->list[cmds->length++] = (Command){tok, paramCounter};
            paramCounter = 1;
        }
    }

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
                return -1 - (int)i;
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
        return (int)jumpIndexStack[--jumpIndexStackHead] + 1;
    }

    return 0;
}

int interpretCmds(char memory[], int dataPtr, Commands *cmds) {
    size_t cmdPtr = 0;
    while (cmdPtr < cmds->length) {
        const Command currCmd = cmds->list[cmdPtr];
        switch (currCmd.tokType) {
        case DP_INC:
            dataPtr += currCmd.param;
            if (dataPtr > MEM_SIZE) {
                return 1;
            }
            break;
        case DP_DEC:
            dataPtr -= currCmd.param;
            if (dataPtr < 0) {
                return -1;
            }
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
                putchar(memory[dataPtr]);
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

    return 0;
}

int runBf(size_t inpLen, const char *inp, char memory[]) {
    Tokens *toks = malloc(sizeof(Tokens) + inpLen * sizeof(TokenType));
    toks->length = 0;

    int tokErr = tokenise(inp, inpLen, toks);

    Commands *cmds = malloc(sizeof(Commands) + toks->length * sizeof(Command));
    cmds->length = 0;

    int parseErr = parse(toks, cmds);

    if (parseErr < 0) {
        fprintf(stderr, "Unbalanced closing bracket found at position %d\n",
                abs(parseErr + 1));
        return 1;
    } else if (parseErr > 0) {
        fprintf(stderr, "Unbalanced opening bracket found at position %d\n",
                parseErr - 1);
        return 1;
    }

    int runErr = interpretCmds(memory, 0, cmds);

    if (runErr != 0) {
        fprintf(stderr, "Data pointer out of bounds!\n");
        return 1;
    }

    return 0;
}

int readFile(const char *filePath, char **out) {
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

#define HELP_TEXT                                                              \
    "Usage:\n"                                                                 \
    "  %s [-h | --help]  Print this help message\n"                            \
    "  %s run <file>     Run a file containing bf code\n"                      \
    "  %s repl           Run bf code interactively in a repl\n"

void showHelp(const char *progName) {
    fprintf(stderr, HELP_TEXT, progName, progName, progName);
}

int main(int argc, char **argv) {
    const char *progName = argv[0];

    if (argc < 2) {
        fprintf(stderr, "Please provide argument(s)!\n\n");
        showHelp(progName);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        showHelp(progName);
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "repl") == 0) {
        printf("cbf: a simple interactive brainfuck interpreter\n"
               "Type `exit` or CTRL-D to exit\n");

        char bfmem[MEM_SIZE] = {0};

        size_t lineSize = 0;
        ssize_t nread;
        char *line = "";
        for (;;) {
            printf("bf> ");

            nread = getline(&line, &lineSize, stdin);
            if (nread == -1 || strcmp(line, "exit\n") == 0) {
                break;
            }
            line[--nread] = 0; // remove newline

            runBf((size_t)nread, line, bfmem);
            putchar('\n');
        }

        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Please provide a file!\n\n");
            showHelp(progName);
            return EXIT_FAILURE;
        }

        const char *fileName = argv[2];
        char *inp;
        int readErr = readFile(fileName, &inp);
        if (readErr != 0) {
            fprintf(stderr, "failed to read file %s: %s\n", fileName,
                    strerror(readErr));
            return EXIT_FAILURE;
        }

        char memory[MEM_SIZE] = {0};
        size_t inpLen = strlen(inp);

        return runBf(inpLen, inp, memory);
    }

    fprintf(stderr, "Provided arguments not recognised!\n\n");
    showHelp(progName);
    return EXIT_FAILURE;
}
