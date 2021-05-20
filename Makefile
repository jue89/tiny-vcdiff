CC=gcc

ODIR=obj
SDIR=src
IDIR=include
TDIR=tests

OBJ = obj/vcdiff_read.o obj/vcdiff_state.o obj/vcdiff.o

CFLAGS=-I$(IDIR)
CFLAGS_TESTS=$(CFLAGS) -lcmocka

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

test_%: $(TDIR)/%.c $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS_TESTS)
