CC=gcc
AR=ar

ODIR=obj
SDIR=src
IDIR=include
TDIR=tests

OBJ = obj/vcdiff_read.o obj/vcdiff_state.o obj/vcdiff_codetable.o obj/vcdiff_addrcache.o obj/vcdiff.o

VCDIFF_BUFFER_SIZE ?= 1024*1024
CFLAGS=-g -Wall -Wextra -Wno-implicit-fallthrough -I$(IDIR) -DVCDIFF_BUFFER_SIZE=$(VCDIFF_BUFFER_SIZE)
CFLAGS_TESTS=$(CFLAGS) -lcmocka

.PHONY: all lib clean tests

all: vcdiff-decode

lib: libvcdiff.a

clean:
	$(RM) $(OBJ)
	$(RM) libvcdiff.a
	$(RM) test_*
	$(RM) vcdiff-decode

test: test_vcdiff_codetable test_vcdiff_read test_vcdiff
	./test_vcdiff_codetable
	./test_vcdiff_read
	./test_vcdiff

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

libvcdiff.a: $(OBJ)
	$(AR) src $@ $^

test_%: $(TDIR)/%.c libvcdiff.a
	$(CC) $(CFLAGS_TESTS) -o $@ $< -L. -lvcdiff

vcdiff-decode: tools/vcdiff-decode.c libvcdiff.a
	$(CC) $(CFLAGS) -o $@ $< -L. -lvcdiff
