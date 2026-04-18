# Vacanti
  Small img viewer fully in C, still working on features for it.

# Dependencies
-  you need ncurses and SDL2:
-  Arch > sudo pacman -S sdl2 sdl2_image ncurses
-  Debian/Any other distro with apt > sudo apt install libsdl2-dev libsdl2-image-dev ibncurses-dev

# Build
  Build it with 
  -  gcc vacanti.c -o vacanti $(sdl2-config --cflags --libs) -lSDL2_image -lncurses
  -  ^Makefile is comining soon dw.
