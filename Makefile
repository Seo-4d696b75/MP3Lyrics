CC = gcc
CFLAGS = -Wall -g
OBJS = charset.o lyrics.o
PROGRAM = lyrics.exe

all:	$(PROGRAM)

$(PROGRAM):	$(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(PROGRAM)
