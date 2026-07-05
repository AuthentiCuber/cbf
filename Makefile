CFLAGS = -Wall -Wextra -Wpedantic -Wconversion -Wtype-limits --std=c99
main:
	$(CC) $(CFLAGS) -o cbf main.c
