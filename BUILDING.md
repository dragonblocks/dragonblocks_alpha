# Building instructions

## Dependencies
You need Git, CMake, Lua, Bash and GCC. Make sure these dependencies are installed on your system. (on Debian based systems: `apt install git cmake build-essential`)

All other dependencies are included as submodules, compiled automatically and statically linked.
Make sure to clone the repository recursively:

```bash
git clone --recurse-submodules https://github.com/dragonblocks/dragonblocks_alpha.git
```

## Client dependencies

If you want to build the client, it is required to install the build dependencies for GLEW and GLFW (on X11/Debian based systems: `apt install xorg-dev libgl1-mesa-dev`).

Refer to:
- https://www.glfw.org/docs/3.3/compile.html
- http://glew.sourceforge.net/build.html

## Building a debug build
By default CMake will make a debug build if nothing else is specified. Simply use

```bash
cmake -B build -S src
cd build
make -j$(nproc)
```

to build the dragonblocks client and server.
If you use a debug build, the singleplayer script should be invoked from the build/ directory, because that's where the binaries are located.

## Building a release snapshot

```bash
# Native snapshot
./snapshot.sh

# Crosscompiling for windows

# win32 (requires i686-w64-mingw32-gcc-posix)
./snapshot.sh win32

# win64 (requires x86_64-w64-mingw32-gcc-posix)
./snapshot.sh win64
```

Creates snapshot zipfiles.
