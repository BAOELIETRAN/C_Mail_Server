files := thread_safe_queue.c thread_safe_queue.h
flags := -g -O2 -std=c2x
CC := gcc

.PHONY = all clean

all: thread_safe_queue

thread_safe_queue: thread_safe_queue.o
	$(CC) $(flags) $^ -o $@

thread_safe_queue.o: $(files)
	$(CC) $(flags) -c $< -o $@

clean:
	rm -f *.o thread_safe_queue 

