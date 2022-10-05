CC=wcl
CFLAGS=-l=dos -bt=dos -fe=bcklght.exe

all:
	$(CC) $(CFLAGS) $(LIBS) bcklght.c

clean:
	rm -f bcklght.o
	rm -f bcklght.exe
