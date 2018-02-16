CFLAGS = -O3 -Wall -Werror -Wpedantic -std=c99
PKG_CONFIG = `pkg-config --cflags --libs i3ipc-glib-1.0 glib-2.0`
SOURCE = i3n.c
EXECUTABLE = i3n

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCE)
	$(CC) $(CFLAGS) $(PKG_CONFIG) $(SOURCE) -o $(EXECUTABLE)

install: $(EXECUTABLE)
	cp $(EXECUTABLE) $(HOME)/bin/$(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE)
