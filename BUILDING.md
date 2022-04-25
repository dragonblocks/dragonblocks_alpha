# Building instructions

## Dependencies
To build anything you need Git, CMake, Lua, Bash and GCC. Make sure these dependencies are installed on your system.
All other dependencies are included as submodules, compiled automatically and statically linked.
Make sure to clone the repository recursively:

```bash
git clone --recurse-submodules https://github.com/dragonblocks/dragonblocks_alpha.git
```

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

# Crosscompiling for windows (requires i686-w64-mingw32-gcc-posix)
./snapshot.sh mingw
```

Creates snapshot zipfiles.
