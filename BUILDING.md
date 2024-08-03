# Building instructions

## Dependencies

- You need Git, Meson, CMake, Lua and GCC. Make sure these dependencies are installed on your system.
	- Debian: `apt-get install git meson cmake build-essential lua5.4`
	- Arch: `pacman -S git meson cmake gcc lua`

- For the client, OpenGL is required:
	- Debian: `apt-get install libgl1-mesa-dev`
	- Arch: `pacman -S mesa`

### Dependencies with fallbacks

These dependencies will be built from source if they are not present.

- ZLib is required by both client and server.
	- Debian: `apt-get install libz-dev`
	- Arch: `pacman -S zlib`

- The server requires SQLite3.
	- Debian: `apt-get install libsqlite3-dev`
	- Arch: `pacman -S sqlite3`

- The client requires Freetype2, GLFW and GLEW.
	- Debian: `apt-get install libglfw3-dev libglew-dev libfreetype-dev`
	- Arch: `pacman -S freetype2 glfw glew`

If you plan to build GLFW and GLEW from source, you still need their build dependencies, refer to:

- https://www.glfw.org/docs/3.3/compile.html
- http://glew.sourceforge.net/build.html

(on X11/Debian based systems: `apt-get install xorg-dev libxkbcommon-dev`)

## Building a debug build

By default Meson will make a debug build. Simply use

```bash
meson setup build
meson compile -C build
```

to build the dragonblocks client and server.
If you use a debug build, the singleplayer script should be invoked from the build/ directory, because that's where the binaries are located.

## Building a statically linked release snapshot

```bash
# Native snapshot
./tools/snapshot.sh

# Crosscompiling for windows

# win32 (requires i686-w64-mingw32-gcc)
./tools/snapshot.sh win32

# win64 (requires x86_64-w64-mingw32-gcc)
./tools/snapshot.sh win64
```

Creates snapshot zipfiles.
