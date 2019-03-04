CC=gcc
CFLAGS=`pkg-config --cflags gtk+-3.0`

ODIR=../build

LIBS=`pkg-config --libs gtk+-3.0`


_OBJ = main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: ui/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

wihotspot: $(OBJ)
	$(CC) -o $(ODIR)/$@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o