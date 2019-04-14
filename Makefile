PREFIX=/usr
MANDIR=$(PREFIX)/share/man
BINDIR=$(PREFIX)/bin

all:
	@echo "Run 'make install' for installation."
	@echo "Run 'make uninstall' for uninstallation."

install:
	$(MAKE) -C src/scripts install

uninstall:
	$(MAKE) -C src/scripts uninstall
