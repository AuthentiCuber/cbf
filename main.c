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

typedef struct {
    unsigned char *cells;
    size_t length;
    size_t dataPtr;
} Memory;

int makeToken(const char c) {
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

int tokenise(const char *input, const size_t inpLen, Tokens *toks) {
    for (size_t i = 0; i < inpLen; i++) {
        int tok = makeToken(input[i]);
        if (tok >= 0) { toks->list[toks->length++] = (TokenType)tok; }
    }
    return 0;
}

/* Parses TOKS into CMDS, combining repeated tokens.
Returns a negative value if an unbalanced closing
bracket is found, or a positive value if an
unbalanced opening bracket is found. */
int parse(const Tokens *toks, Commands *cmds) {
    // combine repeated toks (except jumps)
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
        case JZ: jumpIndexStack[jumpIndexStackHead++] = i; break;
        case JNZ:
            // trying to pop from empty stack
            if (jumpIndexStackHead == 0) { return -1 - (int)i; }

            size_t jumpPos = jumpIndexStack[--jumpIndexStackHead];
            cmds->list[i].param = jumpPos;
            cmds->list[jumpPos].param = i;
            break;
        default: break;
        }
    }
    // leftover items in stack
    if (jumpIndexStackHead != 0) {
        return (int)jumpIndexStack[--jumpIndexStackHead] + 1;
    }

    return 0;
}

/* Execute CMDS given MEMORY of lengith MEMSIZE and the current
DATAPTR position. On success returns the number of characters printed,
a negative return means that DATAPTR went out of bounds. */
int interpretCmds(Memory *mem, const Commands *cmds) {
    size_t cmdPtr = 0;
    int numPrinted = 0;
    while (cmdPtr < cmds->length) {
        const Command currCmd = cmds->list[cmdPtr];
        switch (currCmd.tokType) {
        case DP_INC:
            mem->dataPtr += currCmd.param;
            if (mem->dataPtr > mem->length) { return -2; }
            break;
        case DP_DEC:
            // unsigned ints, cannot check for < 0 after decrement
            if (mem->dataPtr < currCmd.param) { return -1; }
            mem->dataPtr -= currCmd.param;
            break;
        case DATA_INC: mem->cells[mem->dataPtr] += currCmd.param; break;
        case DATA_DEC: mem->cells[mem->dataPtr] -= currCmd.param; break;
        case INPUT:    mem->cells[mem->dataPtr] = (unsigned char)getchar(); break;
        case OUTPUT:
            for (size_t i = 0; i < currCmd.param; i++) {
                putchar(mem->cells[mem->dataPtr]);
                numPrinted++;
            }
            break;
        case JZ:
            if (mem->cells[mem->dataPtr] == 0) { cmdPtr = currCmd.param; }
            break;
        case JNZ:
            if (mem->cells[mem->dataPtr] != 0) { cmdPtr = currCmd.param; }
            break;
        }

        cmdPtr++;
    }

    return numPrinted;
}

/* Runs INP as bf code, given MEMORY, DATAPTR, etc. Like interpretCmds,
 returns number of characters printed and a negative value on error. */
int runBf(const size_t inpLen, const char *inp, Memory *mem) {
    Tokens toks = (Tokens){calloc(inpLen, sizeof(TokenType)), 0};

    tokenise(inp, inpLen, &toks);

    Commands cmds = (Commands){calloc(toks.length, sizeof(Command)), 0};

    int parseErr = parse(&toks, &cmds);

    if (parseErr < 0) {
        fprintf(stderr, "Unbalanced closing bracket found at position %d\n",
                abs(parseErr + 1));
        return -1;
    } else if (parseErr > 0) {
        fprintf(stderr, "Unbalanced opening bracket found at position %d\n",
                parseErr - 1);
        return -1;
    }

    int charsPrinted = interpretCmds(mem, &cmds);

    if (charsPrinted < 0) {
        fprintf(stderr, "Data pointer out of bounds!\n");
        return -1;
    }

    return charsPrinted;
}

/* Helper function that reads the entirety of the
 file at FILEPATH into the string pointed to by OUT. */
int readFile(const char *filePath, char **out) {
    FILE *file = fopen(filePath, "r");

    if (file == NULL) { return errno; }
    if (fseek(file, 0, SEEK_END) < 0) { return errno; }

    unsigned long fileLength = (unsigned long)ftell(file);

    if (fileLength <= 0) { return errno; }
    if (fseek(file, 0, SEEK_SET) < 0) { return errno; }

    char *data = malloc(fileLength);

    if (data == NULL) { return errno; }
    if (fread(data, 1, fileLength, file) < fileLength) { return ferror(file); }
    if (data == NULL) {
        perror("filling input buffer failed");
        return 1;
    }
    // null terminate
    data[fileLength] = 0;

    if (fclose(file) == EOF) { return errno; }

    *out = data;

    return 0;
}

#define HELP_TEXT                                                      \
    "Usage:\n"                                                         \
    "  %s [OPTIONS] run <file>  Run a file containing bf code\n"       \
    "  %s [OPTIONS] repl        Run bf code interactively in a repl\n" \
    "\n"                                                               \
    "Options:\n"                                                       \
    "  --help, -h     Print this help message\n"                       \
    "  --memsize, -m  Set the maximum bf memory tape size\n"

void showHelp(const char *progName) {
    fprintf(stderr, HELP_TEXT, progName, progName);
}

int main(int argc, char **argv) {
    int argIdx = 0;
    const char *progName = argv[argIdx++];

    if (argc < 2) {
        fprintf(stderr, "Please provide argument(s)!\n\n");
        showHelp(progName);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[argIdx], "--help") == 0 ||
        strcmp(argv[argIdx], "-h") == 0) {
        showHelp(progName);
        return EXIT_SUCCESS;
    }

    size_t bfMemSize = 30000;

    if (strcmp(argv[argIdx], "--memsize") == 0 ||
        strcmp(argv[argIdx], "-m") == 0) {
        argIdx++;
        bfMemSize = (size_t)strtol(argv[argIdx++], NULL, 10);
    }

    if (strcmp(argv[argIdx], "repl") == 0) {
        argIdx++;
        printf("cbf: a simple interactive brainfuck interpreter\n"
               "(memory tape %zu x 1 byte cells)\n"
               "Type `exit` or CTRL-D to exit\n",
               bfMemSize);

        Memory bfmem = (Memory){calloc(bfMemSize, 1), bfMemSize, 0};

        char line[200];
        for (;;) {
            printf("bf> ");

            fflush(stdout);
            const char *lineErr = fgets(line, sizeof(line), stdin);
            if (lineErr == NULL || strcmp(line, "exit\n") == 0) {
                if (feof(stdin)) { putchar('\n'); }
                break;
            }

            int charsPrinted = runBf(strlen(line), line, &bfmem);

            if (charsPrinted != 0) { putchar('\n'); }
        }
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[argIdx], "run") == 0) {
        argIdx++;
        if (argc < 3) {
            fprintf(stderr, "Please provide a file!\n\n");
            showHelp(progName);
            return EXIT_FAILURE;
        }

        const char *fileName = argv[argIdx++];
        char *inp;
        int readErr = readFile(fileName, &inp);
        if (readErr != 0) {
            fprintf(stderr, "failed to read file %s: %s\n", fileName,
                    strerror(readErr));
            return EXIT_FAILURE;
        }

        Memory bfMem = (Memory){calloc(bfMemSize, 1), bfMemSize, 0};
        size_t inpLen = strlen(inp);

        int numCharsPrinted = runBf(inpLen, inp, &bfMem);
        if (numCharsPrinted < 0) { return numCharsPrinted; }
        return 0;
    }

    fprintf(stderr, "Provided arguments not recognised!\n\n");
    showHelp(progName);
    return EXIT_FAILURE;
}
