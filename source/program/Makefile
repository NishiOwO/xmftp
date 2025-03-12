CC=gcc
CFLAGS=-Wall

all: misc.o ftp.o systems.o store_options.o url.o other

other:
	@cd unix; \
	$(MAKE);
	@cd site_db; \
	$(MAKE);

include depend

clean:
	@cd unix; \
	$(MAKE) clean;
	@cd site_db; \
	$(MAKE) clean;
	rm -f *.o
