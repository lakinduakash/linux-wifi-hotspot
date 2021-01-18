PREFIX=/usr
MANDIR=$(PREFIX)/share/man
BINDIR=$(PREFIX)/bin

all:
	@echo "Run 'make install' for installation."
	@echo "Run 'make uninstall' for uninstallation."

install:
	install -CDm755 create_ap $(DESTDIR)$(BINDIR)/create_ap
	install -CDm644 create_ap.conf $(DESTDIR)/etc/create_ap.conf
	install -CDm644 create_ap.service $(DESTDIR)$(PREFIX)/lib/systemd/system/create_ap.service
	install -CDm644 bash_completion $(DESTDIR)$(PREFIX)/share/bash-completion/completions/create_ap
	install -CDm644 README.md $(DESTDIR)$(PREFIX)/share/doc/create_ap/README.md
	install -CDm755 wihotspot $(DESTDIR)$(BINDIR)/wihotspot

install-cli-only:
	install -CDm755 create_ap $(DESTDIR)$(BINDIR)/create_ap
	install -CDm644 create_ap.conf $(DESTDIR)/etc/create_ap.conf
	install -CDm644 create_ap.service $(DESTDIR)$(PREFIX)/lib/systemd/system/create_ap.service
	install -CDm644 bash_completion $(DESTDIR)$(PREFIX)/share/bash-completion/completions/create_ap
	install -CDm644 README.md $(DESTDIR)$(PREFIX)/share/doc/create_ap/README.md

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/create_ap
	rm -f $(DESTDIR)/etc/create_ap.conf
	rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/create_ap.service
	rm -f $(DESTDIR)$(PREFIX)/share/bash-completion/completions/create_ap
	rm -f $(DESTDIR)$(PREFIX)/share/doc/create_ap/README.md
	rm -f $(DESTDIR)$(BINDIR)/wihotspot
