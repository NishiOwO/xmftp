CC=gcc
CFLAGS=-Wall $(INC_FLAGS)

all: ftplib.o

screwy_gethostbyname: doit

doit:
	$(CC) $(CFLAGS) -DBAD_GETHOSTBYNAME -c ftplib.c

include depend

clean:
	rm -f *.o
