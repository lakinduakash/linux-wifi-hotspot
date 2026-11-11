all:
	mkdir -p build
	@echo "Run 'sudo make install' for installation."
	@echo "Run 'sudo make uninstall' for uninstallation."
	$(MAKE) -C src

install:
	@echo "Installing..."
	$(MAKE) -C src install

release:
	mkdir -p build
	@echo "Building release binary..."
	$(MAKE) -C src release

test:
	mkdir -p build
	@echo "Testing..."
	$(MAKE) -C test

install-cli-only:
	@echo "Installing command line interface only..."
	$(MAKE) -C src/scripts install-cli-only

uninstall:
	@echo "Uninstalling..."
	$(MAKE) -C src uninstall

clean-old:
	$(MAKE) -C src clean-old

.PHONY: clean test

clean:
	$(MAKE) -C src clean
	$(MAKE) -C test clean
