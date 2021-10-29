# Building instructions

CMake is used for compiling. Make sure to clone the repository recursively:

```bash
git clone --recurse-submodules https://github.com/dragonblocks/dragonblocks_alpha.git
```

## Dependencies
To build anything you need CMake. The ZLib development library is needed as well.
The development versions of OpenGL, GLFW3, GLEW and Freetype are required to build the client.
For building the server the SQLite3 development library is required.


Ubuntu / Debian:

```bash
sudo apt install build-essential cmake zlib1g-dev libgl1-mesa-dev libglfw3-dev libglew-dev libfreetype-dev libsqlite3-dev
```

FreeBSD:

```csh
sudo pkg install cmake gcc lzlib mesa-devel glfw glew freetype sqlite3
```

OpenBSD:

```sh
sudo pkg_add cmake lzlib glfw glew freetype sqlite3
```

## Building a debug build
By default CMake will make a Debug build if nothing else is specified. Simply use

```bash
cd src
cmake .
make -j$(nproc)
```

to build the dragonblocks client and server.
If you use a debug build, the singleplayer script should be invoked from the src/ directory, because that's where the binaries are located.

## Building a snapshot

```bash
./snapshot.sh
```
This script will create a snapshot zipfile.
