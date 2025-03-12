# Where to install the xmftp binary
INSTALL_BIN_DIR=/usr/local/bin

# If your libraries (see LIBS below) are located somewhere else, change this
LFLAGS=-L/usr/X11R7/lib -L/usr/pkg/lib

# Shouldn't have to edit anything below this line
##########################################################################

CC=gcc
CFLAGS=-Wall -I/usr/X11R7/include -I/usr/pkg/include
OBJ=	ui/xmftp.o ui/build_menu.o ui/menu_creation.o ui/operations.o \
	ui/layout.o ui/setup_cbs.o program/misc.o \
	program/unix/implementation.o program/ftp.o \
	ftplib/ftplib.o ui/remote_refresh.o ui/download.o \
	ui/site_mgr.o program/site_db/site_db.o ui/toolbar.o \
	ui/fill_filelist.o ui/transfer.o ui/upload.o ui/view.o \
	ui/local_remote_ops.o ui/connection.o program/systems.o \
	ui/pixmap.o ui/options.o program/store_options.o \
	program/url.o ui/password.o
LIBS=		-lXm -lXt -lXpm -lXext -lX11
#STATIC_LIBS=	/usr/X11R6/lib/libXm.a -lXt -lSM -lICE -lXpm -lXext -lX11
STATIC_LIBS=	/usr/X11R6/lib/libXm.a -lXt -lXpm -lXext -lX11
VERSION=1.0.4

all: xmftp

bad_gethostbyname: doobj
	cd ftplib; \
	$(MAKE) screwy_gethostbyname;
	$(CC) $(CFLAGS) $(LFLAGS) -o xmftp $(OBJ) $(LIBS)		
	@echo ""; \
	echo "All done! You may do 'make install' now if you like."; \
	echo ""

xmftp: doobj
	$(CC) $(CFLAGS) $(LFLAGS) -o xmftp $(OBJ) $(LIBS)
	@echo ""; \
	echo "All done! You may do 'make install' now if you like."; \
	echo ""

static: doobj
	$(CC) $(CFLAGS) $(LFLAGS) -o xmftp $(OBJ) $(STATIC_LIBS)
	strip xmftp
	@echo ""; \
	echo "All done! You may do 'make install' now if you like."; \
	echo ""

doobj: 
	@cd ui; \
	$(MAKE); 
	@cd program; \
	$(MAKE); 
	@cd ftplib; \
	$(MAKE);

clean:
	@cd ui; \
	$(MAKE) clean; 
	@cd program; \
	$(MAKE) clean; 
	@cd ftplib; \
	$(MAKE) clean;
	rm -f xmftp

superclean: clean
	find . -iname '*~' -exec rm -f \{\} \;

package: superclean	
	cd ../..; \
	tar czf xmftp-$(VERSION).tar.gz xmftp;
	$(MAKE);
#	cd ftplib; \
#	$(MAKE) screwy_gethostbyname;
#	$(MAKE);
	cd ../..; \
	tar czf xmftp-$(VERSION).bin.ELF.shared.tar.gz xmftp;
	$(MAKE) static;
	cd ../..; \
	tar czf xmftp-$(VERSION).bin.ELF.static.tar.gz xmftp;
	rm -f xmftp

install: uninstall
	install -m 0755 -s xmftp $(INSTALL_BIN_DIR)

uninstall:
	rm -f $(INSTALL_BIN_DIR)/xmftp
