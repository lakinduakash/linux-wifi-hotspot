CC=gcc
PKGCONFIG = $(shell which pkg-config)

CFLAGS=`pkg-config --cflags gtk+-3.0` -I./../src/ui

LIBS=`pkg-config --libs gtk+-3.0 --libs x11` -lstdc++ -lpng -lqrencode


ODIR=./../build

# Determine this makefile's path.
# Be sure to place this BEFORE `include` directives, if any.
THIS_FILE := $(lastword $(MAKEFILE_LIST))

_OBJ = util.o test_util.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


.PHONY: clean

all: $(OBJ) test


$(ODIR)/util.o: ../src/ui/util.c
	$(CC) -c $? -o $@ $(CFLAGS)

$(ODIR)/test_util.o: test_util.c
	$(CC) -c $? -o $@ $(CFLAGS)

test: $(OBJ)
	$(CC) -o $(ODIR)/test $^
	@$(ODIR)/test

clean:
	rm -f $(OBJ)
	rm -f $(ODIR)/test

