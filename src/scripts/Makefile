DESTDIR?=/usr

all:
	@echo "Run 'make install' for installation."
	@echo "Run 'make uninstall' for uninstallation."

install:
	install -CDm755 create_ap $(DESTDIR)/bin/create_ap
	install -CDm644 create_ap.conf $(DESTDIR)/etc/create_ap.conf
	install -CDm644 create_ap.service $(DESTDIR)/lib/systemd/system/create_ap.service
	install -CDm644 bash_completion $(DESTDIR)/share/bash-completion/completions/create_ap
	install -CDm644 README.md $(DESTDIR)/share/doc/create_ap/README.md
	install -CDm755 wihotspot $(DESTDIR)/bin/wihotspot

install-cli-only:
	install -CDm755 create_ap $(DESTDIR)/bin/create_ap
	install -CDm644 create_ap.conf $(DESTDIR)/etc/create_ap.conf
	install -CDm644 create_ap.service $(DESTDIR)/lib/systemd/system/create_ap.service
	install -CDm644 bash_completion $(DESTDIR)/share/bash-completion/completions/create_ap
	install -CDm644 README.md $(DESTDIR)/share/doc/create_ap/README.md

uninstall:
	rm -f $(DESTDIR)/bin/create_ap
	rm -f $(DESTDIR)/etc/create_ap.conf
	rm -f $(DESTDIR)/lib/systemd/system/create_ap.service
	rm -f $(DESTDIR)/share/bash-completion/completions/create_ap
	rm -f $(DESTDIR)/share/doc/create_ap/README.md
	rm -f $(DESTDIR)/bin/wihotspot
