PREFIX ?= /usr
LIBEXECDIR ?= $(PREFIX)/lib/linux-wifi-hotspot
SYSTEMD_DIR ?= $(PREFIX)/lib/systemd/system
CARGO ?= cargo
NPM ?= npm

all:
	mkdir -p build
	@echo "Run 'sudo make install' for installation."
	@echo "Run 'sudo make uninstall' for uninstallation."
	cd src && $(MAKE)

install:
	@echo "Installing..."
	cd src && $(MAKE) install
	cd services/hotspotd && $(CARGO) build --release
	install -Dm755 services/hotspotd/target/release/hotspotd $(DESTDIR)$(LIBEXECDIR)/hotspotd
	install -Dm644 services/hotspotd/hotspotd.service $(DESTDIR)$(SYSTEMD_DIR)/hotspotd.service
	install -Dm644 services/hotspotd/assets/create_ap.conf $(DESTDIR)$(PREFIX)/share/linux-wifi-hotspot/create_ap.conf
	@mkdir -p $(DESTDIR)/etc/linux-wifi-hotspot
	@if [ ! -f $(DESTDIR)/etc/linux-wifi-hotspot/hotspotd.env ]; then \
		install -Dm640 services/hotspotd/hotspotd.env.example $(DESTDIR)/etc/linux-wifi-hotspot/hotspotd.env; \
	else \
		echo "Skipping /etc/linux-wifi-hotspot/hotspotd.env (already exists)"; \
	fi

test:
	mkdir -p build
	@echo "Testing..."
	cd test && $(MAKE)

install-cli-only:
	@echo "Installing command line interface only..."
	cd src/scripts && $(MAKE) install-cli-only

uninstall:
	@echo "Uninstalling..."
	cd src && $(MAKE) uninstall
	rm -f $(DESTDIR)$(LIBEXECDIR)/hotspotd
	rm -f $(DESTDIR)$(SYSTEMD_DIR)/hotspotd.service

clean-old:
	cd src && $(MAKE) clean-old

.PHONY: clean test

clean:
	cd src && $(MAKE) clean
	cd test && $(MAKE) clean
	cd services/hotspotd && $(CARGO) clean

webui-build:
	cd webui && $(NPM) ci
	cd webui && $(NPM) run build
