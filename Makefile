all:
	@echo "Run 'make install' for installation."
	@echo "Run 'make uninstall' for uninstallation."
	mkdir -p build
	cd build && cmake -G "CodeBlocks - Unix Makefiles" ../src

install:
	@echo "Installing"
	cd build
	whereis cmake
	/home/travis/usr/bin/cmake /usr/local/cmake-3.12.4/bin/cmake -DCMAKE_INSTALL_PREFIX=$(DESTDIR) -G "CodeBlocks - Unix Makefiles" ../src
	$(MAKE) -C src/scripts install
	$(MAKE) -C build install_build

uninstall:
	$(MAKE) -C src/scripts uninstall
	$(MAKE) -C build uninstall_build
