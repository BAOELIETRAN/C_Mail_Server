files := mail_user_agent.c mail_user_agent.h
flags := -O2 -std=c2x
CC := gcc

.PHONY = all clean

all: mail_user_agent

mail_user_agent: mail_user_agent.o
	$(CC) $(flags) $^ -o $@

mail_user_agent.o: $(files)
	$(CC) $(flags) -c $< -o $@

clean:
	rm -f *.o mail_user_agent email-*

