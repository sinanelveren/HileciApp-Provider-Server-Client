CC = gcc
DB = gdb
CFLAGS = -o
DFLAGS = -g
MAIN = 111044074_main
CLIENT = 111044074_client
USAGE = usage.txt
PROGNAME = homeworkServer
CLIPROG = clientApp


all:
	clear
	clear
	$(CC) -std=c11 -c $(MAIN).c -lpthread -lm
	$(CC) $(MAIN).o -lpthread  $(CFLAGS) $(PROGNAME) -lm
	$(CC) -std=c11 -lm -c  $(CLIENT).c -lpthread -lm
	$(CC) $(CLIENT).o -lpthread -lm  $(CFLAGS) $(CLIPROG)
	cat $(USAGE)
	

debug:
	$(CC) -std=c11 $(DFLAGS) $(MAIN).c $(CFLAGS) $(PROGNAME)
	$(DB) ./$(PROGNAME)


clean:
	rm -f $(PROGNAME) $(CLIPROG) *.o *.log
