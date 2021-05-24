CC=gcc

ODIR=obj
SDIR=src
IDIR=include
TDIR=tests

OBJ = obj/vcdiff_read.o obj/vcdiff_state.o obj/vcdiff_codetable.o obj/vcdiff_addrcache.o obj/vcdiff.o

VCDIFF_BUFFER_SIZE ?= 16*1024
CFLAGS=-I$(IDIR) -DVCDIFF_BUFFER_SIZE=$(VCDIFF_BUFFER_SIZE)
CFLAGS_TESTS=$(CFLAGS) -lcmocka

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -g -c -o $@ $< $(CFLAGS)

test_%: $(TDIR)/%.c $(OBJ)
	$(CC) -g -o $@ $^ $(CFLAGS_TESTS)
