CC = g++

CFLAGS = -g -Wall -std=c++17
TARGET = ideapad_manager

GTK_FLAGS = -I/usr/include/gtk-3.0 -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/sysprof-4 -I/usr/include/harfbuzz -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/cairo -I/usr/include/lzo -I/usr/include/pixman-1 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/gio-unix-2.0 -I/usr/include/cloudproviders -I/usr/include/atk-1.0 -I/usr/include/at-spi2-atk/2.0 -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include -I/usr/include/at-spi-2.0 -pthread -lgtk-3 -lgdk-3 -lz -lpangocairo-1.0 -lpango-1.0 -lharfbuzz -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0
INDICATOR_FLAGS = -I/usr/include/libappindicator3-0.1 -I/usr/include/libdbusmenu-glib-0.4 -lappindicator3 -ldbusmenu-glib

all: $(TARGET)

$(TARGET): main.cpp
		$(CC) -o $(TARGET) main.cpp $(CFLAGS) $(GTK_FLAGS) $(INDICATOR_FLAGS)
clean:
		$(RM) $(TARGET)
build:
		$(MAKE) $(TARGET)
run:
		$(MAKE) clean
		$(MAKE) build
		./ideapad_manager
release:
		cp $(TARGET) ~/bin
autostart:
		cp $(TARGET).desktop ~/.config/autostart
