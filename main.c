#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
} TokenType;

MAKE_ARRAY_TYPE(TokenType, tokArr);

typedef struct {
    TokenType tokType;
    union {
        size_t jumploc; // for jz, jnz
        int numtimes;   // otherwise
    };
} Command;

MAKE_ARRAY_TYPE(Command, cmdArr);

typedef struct {
    unsigned char *cells;
    int length;
    int dataPtr;
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

int tokenise(const char *input, const size_t inpLen, tokArr *toks) {
    for (size_t i = 0; i < inpLen; i++) {
        int tok = makeToken(input[i]);
        if (tok >= 0) { toks->items[toks->count++] = (TokenType)tok; }
    }
    return 0;
}

int canCollapse(TokenType a, TokenType b) {
    switch (a) {
    case JZ: // FALLTHROUGH
    case JNZ: return 0;
    case DP_INC:
        if (b == DP_DEC) { return -1; }
        break;
    case DP_DEC:
        if (b == DP_INC) { return -1; }
        break;
    case DATA_INC:
        if (b == DATA_DEC) { return -1; }
        break;
    case DATA_DEC:
        if (b == DATA_INC) { return -1; }
        break;
    default: break;
    }
    return a == b;
}

/* Parses TOKS into CMDS, combining collapsable tokens.
Returns a negative value if an unbalanced closing
bracket is found, or a positive value if an
unbalanced opening bracket is found. */
int parse(tokArr *toks, cmdArr *cmds) {
    // combine collapsable tokens
    int paramCounter = 1;
    size_t tokScanner = 0;
    while (tokScanner < toks->count) {
        TokenType currTok = toks->items[tokScanner];
        for (;;) {
            if (tokScanner++ >= toks->count) { break; }

            int collapseSign = canCollapse(currTok, toks->items[tokScanner]);
            if (collapseSign == 0) { break; }

            paramCounter += collapseSign;
        }
        cmds->items[cmds->count++] =
            (Command){currTok, {.numtimes = paramCounter}};
        paramCounter = 1;
    }

    // jump location resolution
    size_t jumpIndexStack[cmds->count * sizeof(int)];
    size_t jumpIndexStackHead = 0;
    for (size_t i = 0; i < cmds->count; i++) {
        switch (cmds->items[i].tokType) {
        case JZ: jumpIndexStack[jumpIndexStackHead++] = i; break;
        case JNZ:
            // trying to pop from empty stack
            if (jumpIndexStackHead == 0) { return -1 - (int)i; }

            size_t jumpPos = jumpIndexStack[--jumpIndexStackHead];
            cmds->items[i].jumploc = jumpPos;
            cmds->items[jumpPos].jumploc = i;
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
int interpretCmds(Memory *mem, cmdArr *cmds) {
    size_t cmdPtr = 0;
    int numPrinted = 0;
    while (cmdPtr < cmds->count) {
        const Command currCmd = cmds->items[cmdPtr];
        switch (currCmd.tokType) {
        case DP_INC:
            mem->dataPtr += currCmd.numtimes;
            if (mem->dataPtr > mem->length) { return -2; }
            break;
        case DP_DEC:
            // unsigned ints, cannot check for < 0 after decrement
            if (mem->dataPtr < currCmd.numtimes) { return -1; }
            mem->dataPtr -= currCmd.numtimes;
            break;
        case DATA_INC: mem->cells[mem->dataPtr] += currCmd.numtimes; break;
        case DATA_DEC: mem->cells[mem->dataPtr] -= currCmd.numtimes; break;
        case INPUT:    mem->cells[mem->dataPtr] = (unsigned char)getchar(); break;
        case OUTPUT:
            for (int i = 0; i < currCmd.numtimes; i++) {
                putchar(mem->cells[mem->dataPtr]);
                numPrinted++;
            }
            break;
        case JZ:
            if (mem->cells[mem->dataPtr] == 0) { cmdPtr = currCmd.jumploc; }
            break;
        case JNZ:
            if (mem->cells[mem->dataPtr] != 0) { cmdPtr = currCmd.jumploc; }
            break;
        }
        cmdPtr++;
    }

    return numPrinted;
}

/* Runs INP as bf code, given MEMORY, DATAPTR, etc. Like interpretCmds,
 returns number of characters printed and a negative value on error. */
int runBf(size_t inpLen, const char *inp, Memory *mem, int debug) {
    tokArr toks = {calloc(inpLen, sizeof(TokenType)), 0};

    tokenise(inp, inpLen, &toks);
    if (debug) {
        printf("------- Generated tokens start -------\n");
        for (size_t i = 0; i < toks.count; i++) {
            printf("%d ", toks.items[i]);
        }
        printf("\n------- Generated tokens end -------\n");
    }

    cmdArr cmds = {calloc(toks.count, sizeof(Command)), 0};

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

    if (debug) {
        printf("------- Generated commands start -------\n");
        for (size_t i = 0; i < cmds.count; i++) {
            // numtimes isnt technically correct here...
            printf("%d: %d\n", cmds.items[i].tokType, cmds.items[i].numtimes);
        }
        printf("------- Generated commands end -------\n");
        printf("------- Output start -------\n");
    }

    int charsPrinted = interpretCmds(mem, &cmds);

    if (debug) { printf("\n------- Output end -------\n"); }

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
    data[fileLength - 1] = 0;

    if (fclose(file) == EOF) { return errno; }

    *out = data;

    return 0;
}

#define HELP_TEXT                                                      \
    "Usage:\n"                                                         \
    "  %s [OPTIONS] run [-d] <file>  Run a file containing bf code\n"  \
    "  %s [OPTIONS] repl        Run bf code interactively in a repl\n" \
    "\n"                                                               \
    "Glabal options:\n"                                                \
    "  --help, -h     Print this help message\n"                       \
    "  --memsize, -m  Set the maximum bf memory tape size\n"           \
    "\n"                                                               \
    "Mode specific options:\n"                                         \
    "  --debug, -d     With `run`, show debug information\n"

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

    int bfMemSize = 30000;

    if (strcmp(argv[argIdx], "--memsize") == 0 ||
        strcmp(argv[argIdx], "-m") == 0) {
        argIdx++;
        bfMemSize = (int)strtol(argv[argIdx++], NULL, 10);
    }

    if (strcmp(argv[argIdx], "repl") == 0) {
        argIdx++;
        printf("cbf: a simple interactive brainfuck interpreter\n"
               "(memory tape %d x 1 byte cells)\n"
               "Type `exit` or CTRL-D to exit\n",
               bfMemSize);

        Memory bfmem = {calloc((size_t)bfMemSize, 1), bfMemSize, 0};

        char line[200];
        for (;;) {
            printf("bf> ");

            fflush(stdout);
            const char *lineErr = fgets(line, sizeof(line), stdin);
            if (lineErr == NULL || strcmp(line, "exit\n") == 0) {
                if (feof(stdin)) { putchar('\n'); }
                break;
            }

            int charsPrinted = runBf(strlen(line), line, &bfmem, 0);

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

        int debug = 0;
        if (strcmp(argv[argIdx], "--debug") == 0 ||
            strcmp(argv[argIdx], "-d") == 0) {
            argIdx++;
            debug = 1;
        }

        const char *fileName = argv[argIdx++];
        char *inp;
        int readErr = readFile(fileName, &inp);
        if (readErr != 0) {
            fprintf(stderr, "failed to read file %s: %s\n", fileName,
                    strerror(readErr));
            return EXIT_FAILURE;
        }

        Memory bfMem = {calloc((size_t)bfMemSize, 1), bfMemSize, 0};
        size_t inpLen = strlen(inp);

        int numCharsPrinted = runBf(inpLen, inp, &bfMem, debug);
        if (numCharsPrinted < 0) { return numCharsPrinted; }
        return 0;
    }

    fprintf(stderr, "Provided arguments not recognised!\n\n");
    showHelp(progName);
    return EXIT_FAILURE;
}
