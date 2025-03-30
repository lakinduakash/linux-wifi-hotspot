all:
	@echo "Run 'make install' for installation."
	@echo "Run 'make uninstall' for uninstallation."
	mkdir -p build
	cd build && cmake -DCMAKE_INSTALL_PREFIX=$(DESTDIR) ../src

install:
	@echo "Installing"
	cd build && cmake -DCMAKE_INSTALL_PREFIX=$(DESTDIR) ../src
	$(MAKE) -C src/scripts install
	$(MAKE) -C build install

uninstall:
	$(MAKE) -C src/scripts uninstall
	$(MAKE) -C build uninstall
