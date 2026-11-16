CC=gcc
PKGCONFIG = $(shell which pkg-config)

CFLAGS += `pkg-config --cflags gtk+-3.0`
LDFLAGS += `pkg-config --libs gtk+-3.0 --libs x11` -lstdc++ -lpng -lqrencode

APP_NAME="wihotspot"
APP_GUI_BINARY="wihotspot-gui"
APP_ADW_BINARY="wihotspot-adw"

ENABLE_GTK3 ?= 1
ENABLE_ADW ?= 1
ifdef BUILD_GTK3
  ENABLE_GTK3 = $(BUILD_GTK3)
endif
ifdef BUILD_ADW
  ENABLE_ADW = $(BUILD_ADW)
endif
ifeq ($(DISABLE_GTK3),1)
  ENABLE_GTK3 = 0
endif
ifeq ($(DISABLE_ADW),1)
  ENABLE_ADW = 0
endif

PREFIX=/usr
BINDIR=$(PREFIX)/bin

ODIR=../build

GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)
BLUEPRINT_COMPILER ?= blueprint-compiler

BUILT_SRC = resources.c

_OBJ = main.o ui.o h_prop.o util.o read_config.o about_ui.o qr_ui.o qrgen.o $(BUILT_SRC:.c=.o)
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

ADW_CFLAGS = -Iui -Iui/adw `pkg-config --cflags libadwaita-1`
ADW_LDFLAGS = `pkg-config --libs libadwaita-1` -lstdc++ -lpng -lqrencode
ADW_OBJ = $(ODIR)/h_prop.o $(ODIR)/util.o $(ODIR)/read_config.o $(ODIR)/qrgen.o \
          $(ODIR)/adw_main.o $(ODIR)/adw_ui.o $(ODIR)/resources_adw.o

TARGETS =
ifneq ($(ENABLE_GTK3),0)
TARGETS += wihotspot-gui
endif
ifneq ($(ENABLE_ADW),0)
TARGETS += wihotspot-adw
endif

# Determine this makefile's path.
# Be sure to place this BEFORE `include` directives, if any.
THIS_FILE := $(lastword $(MAKEFILE_LIST))

.PHONY: clean

all: $(TARGETS)

ui/resources.c: ui/glade/wifih.gresource.xml ui/glade/wifih.ui
	$(GLIB_COMPILE_RESOURCES) ui/glade/wifih.gresource.xml --target=$@ --sourcedir=ui/glade --generate-source

$(ODIR)/resources.o: ui/resources.c

$(ODIR)/%.o: ui/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: ui/%.cpp
	g++ -c -o $@ $< $(CXXFLAGS)

wihotspot-gui: $(OBJ)
	$(CC) -o $(ODIR)/$@ $^ $(CFLAGS) $(LDFLAGS)

ui/adw/window.ui: ui/adw/blp/window.blp
	$(BLUEPRINT_COMPILER) compile $< --output $@

ui/adw/resources_adw.c: ui/adw/resources/wihotspot-adw.gresource.xml ui/adw/window.ui
	$(GLIB_COMPILE_RESOURCES) $< --target=$@ --sourcedir=ui/adw --generate-source

$(ODIR)/adw_main.o: ui/adw/main.c
	@mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(ADW_CFLAGS)

$(ODIR)/adw_ui.o: ui/adw/adw_ui.c
	@mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(ADW_CFLAGS)

$(ODIR)/resources_adw.o: ui/adw/resources_adw.c
	@mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(ADW_CFLAGS)

wihotspot-adw: $(ADW_OBJ)
	$(CC) -o $(ODIR)/$@ $^ $(ADW_LDFLAGS)

install:
	install -Dm644 desktop/icons/hotspot@64.png $(DESTDIR)$(PREFIX)/share/pixmaps/wihotspot.png
	install -Dm644 desktop/icons/hotspot@48.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/wihotspot.png
	install -Dm644 desktop/icons/hotspot@64.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/64x64/apps/wihotspot.png
	install -Dm644 desktop/icons/hotspot@256.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/wihotspot.png
	install -Dm644 desktop/icons/hotspot@512.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/512x512/apps/wihotspot.png
	install -Dm644 desktop/icons/hotspot.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/wihotspot.svg
	install -Dm644 desktop/wifihotspot.desktop $(DESTDIR)$(PREFIX)/share/applications/$(APP_NAME).desktop
	install -Dm644 scripts/policies/polkit.rules $(DESTDIR)$(PREFIX)/share/polkit-1/rules.d/90-org.opensuse.policykit.wihotspot.rules
	install -Dm644 scripts/policies/polkit.policy $(DESTDIR)$(PREFIX)/share/polkit-1/actions/org.opensuse.policykit.wihotspot.policy
	if [ -f $(ODIR)/wihotspot-gui ]; then \
		install -Dm755 $(ODIR)/wihotspot-gui $(DESTDIR)$(BINDIR)/$(APP_GUI_BINARY); \
	fi
	if [ -f $(ODIR)/wihotspot-adw ]; then \
		install -Dm755 $(ODIR)/wihotspot-adw $(DESTDIR)$(BINDIR)/$(APP_ADW_BINARY); \
	fi
	cd scripts && $(MAKE) install

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/share/pixmaps/wihotspot.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/wihotspot.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/64x64/apps/wihotspot.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/wihotspot.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/512x512/apps/wihotspot.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/wihotspot.svg
	rm -f $(DESTDIR)$(PREFIX)/share/applications/$(APP_NAME).desktop
	rm -f $(DESTDIR)$(PREFIX)/share/polkit-1/rules.d/90-org.opensuse.policykit.wihotspot.rules
	rm -f $(DESTDIR)$(PREFIX)/share/polkit-1/actions/org.opensuse.policykit.wihotspot.policy
	rm -f $(DESTDIR)$(BINDIR)/$(APP_GUI_BINARY)
	rm -f $(DESTDIR)$(BINDIR)/$(APP_ADW_BINARY)
	cd scripts && $(MAKE) uninstall

clean-old:
	rm -f $(DESTDIR)$(PREFIX)/share/pixmaps/wihotspot.png

clean:
	rm -f $(ODIR)/*.o
	rm -f ui/$(BUILT_SRC)
	rm -f ui/adw/resources_adw.c ui/adw/window.ui
	rm -f $(ODIR)/wihotspot-gui $(ODIR)/wihotspot-adw
