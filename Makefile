all:
	mkdir -p build
	@echo "Run 'sudo make install' for installation."
	@echo "Run 'sudo make uninstall' for uninstallation."
	cd src && $(MAKE)

install:
	@echo "Installing..."
	cd src && $(MAKE) install

install-cli-only:
	@echo "Installing command line interface only..."
	cd src/scripts && $(MAKE) install-cli-only

uninstall:
	@echo "Uninstalling..."
	cd src && $(MAKE) uninstall

clean-old:
	cd src && $(MAKE) clean-old

.PHONY: clean

clean:
	cd src && $(MAKE) clean
