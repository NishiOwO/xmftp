CC=gcc
CFLAGS=-Wall

all: ftplib.o

screwy_gethostbyname: doit

doit:
	$(CC) $(CFLAGS) -DBAD_GETHOSTBYNAME -c ftplib.c

include depend

clean:
	rm -f *.o
