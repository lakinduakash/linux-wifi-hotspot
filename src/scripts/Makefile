PREFIX=/usr
MANDIR=$(PREFIX)/share/man
BINDIR=$(PREFIX)/bin

all:
	@echo "Run 'make install' for installation."
	@echo "Run 'make uninstall' for uninstallation."

install:
	# echo "HERE: $(HOME)"
	install -Dm755 create_ap $(DESTDIR)$(BINDIR)/create_ap
	install -Dm644 create_ap.conf $(DESTDIR)/etc/create_ap.conf
	[ ! -d /lib/systemd/system ] || install -Dm644 create_ap.service $(DESTDIR)$(PREFIX)/lib/systemd/system/create_ap.service
	install -Dm644 bash_completion $(DESTDIR)$(PREFIX)/share/bash-completion/completions/create_ap
	install -Dm644 README.md $(DESTDIR)$(PREFIX)/share/doc/create_ap/README.md
	install -Dm644 hotspot.png $(DESTDIR)$(PREFIX)/share/doc/create_ap/hotspot.png
	install -Dm644 wifihotspot.desktop $(DESTDIR)$(PREFIX)/share/doc/create_ap/wifihotspot.desktop
	install -Dm644 wifihotspot.desktop $(DESTDIR)$(PREFIX)/share/applications/wifihotspot.desktop


uninstall:
	rm -f $(DESTDIR)$(BINDIR)/create_ap
	rm -f $(DESTDIR)/etc/create_ap.conf
	[ ! -f /lib/systemd/system/create_ap.service ] || rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/create_ap.service
	rm -f $(DESTDIR)$(PREFIX)/share/bash-completion/completions/create_ap
	rm -f $(DESTDIR)$(PREFIX)/share/doc/create_ap/README.md
	rm -f $(DESTDIR)$(PREFIX)/share/doc/create_ap/hotspot.png
	rm -f $(DESTDIR)$(PREFIX)/share/doc/create_ap/wifihotspot.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/wifihotspot.desktop
