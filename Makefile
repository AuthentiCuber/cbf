CFLAGS = -Wall -Wextra -Wpedantic --std=c99
main:
	$(CC) $(CFLAGS) -o cbf main.c
