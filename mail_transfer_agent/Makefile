files := mail_transfer_agent.c mail_transfer_agent.h
flags := -g -O2 -std=c2x
CC := gcc

.PHONY = all clean

all: mail_transfer_agent

mail_submission_agent: mail_transfer_agent.o
	$(CC) $(flags) $^ -o $@

mail_submission_agent.o: $(files)
	$(CC) $(flags) -c $< -o $@

clean:
	rm -f *.o mail_transfer_agent 

