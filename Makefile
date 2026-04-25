CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell sdl2-config --cflags)
LIBS = -lSDL2_image -lncurses $(shell sdl2-config --libs)
TARGET = vacanti
SRC = vacanti.c

# Default prefix could be replaced with 'sudo make install PREFIX=/your/path
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	@echo "Installing $(TARGET) to $(BINDIR)..."
	sudo mkdir -p $(BINDIR)
	sudo cp $(TARGET) $(BINDIR)/
	sudo chmod 755 $(BINDIR)/$(TARGET)
	@echo "Installed successfully!"

uninstall:
	@echo "Removing $(TARGET) from $(BINDIR)..."
	sudo rm -f $(BINDIR)/$(TARGET)
	@echo "Uninstalled successfully!"

.PHONY: all clean install uninstall
