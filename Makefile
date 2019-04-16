all:
	@echo "Run 'make install' for installation."
	@echo "Run 'make uninstall' for uninstallation."
	mkdir -p build
	cd build && cmake -G "CodeBlocks - Unix Makefiles" ../src
	$(MAKE) -C build

install:
	$(MAKE) -C src/scripts install
	$(MAKE) -C build install_build

uninstall:
	$(MAKE) -C src/scripts uninstall
	$(MAKE) -C build uninstall_build
