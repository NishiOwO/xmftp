# You may have to uncomment the following line and set it appropriately
# to point to the location of the X and/or Motif headers
#INC_FLAGS=-I/some/dir/where/headers/are

#########################################################################

CC=gcc
CFLAGS=-Wall $(INC_FLAGS)
OBJ=	xmftp.o build_menu.o layout.o menu_creation.o operations.o \
	setup_cbs.o remote_refresh.o download.o site_mgr.o toolbar.o \
	fill_filelist.o transfer.o upload.o view.o local_remote_ops.o \
	connection.o pixmap.o options.o password.o

all: $(OBJ)

include depend

clean:
	rm -f *.o

