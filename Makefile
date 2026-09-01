CFLAGS = -Wall -Wextra -Wpedantic -Wconversion -Wtype-limits --std=c11
main:
	$(CC) $(CFLAGS) -o cbf main.c libcbf.c
