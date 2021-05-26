CC=gcc
AR=ar

ODIR=obj
SDIR=src
IDIR=include
TDIR=tests

OBJ = obj/vcdiff_read.o obj/vcdiff_state.o obj/vcdiff_codetable.o obj/vcdiff_addrcache.o obj/vcdiff.o

VCDIFF_BUFFER_SIZE ?= 1024*1024
CFLAGS=-g -I$(IDIR) -DVCDIFF_BUFFER_SIZE=$(VCDIFF_BUFFER_SIZE)
CFLAGS_TESTS=$(CFLAGS) -lcmocka

.PHONY: lib clean tests

lib: libvcdiff.a

clean:
	rm $(OBJ)
	rm libvcdiff.a
	rm test_*

test: test_vcdiff_codetable test_vcdiff_read test_vcdiff
	./test_vcdiff_codetable
	./test_vcdiff_read
	./test_vcdiff

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

libvcdiff.a: $(OBJ)
	$(AR) src $@ $^

test_%: $(TDIR)/%.c libvcdiff.a
	$(CC) -o $@ $< $(CFLAGS_TESTS) -L. -lvcdiff
