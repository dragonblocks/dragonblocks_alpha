# Building instructions

CMake is used for compiling. Make sure to clone the repository recursively:

```bash
git clone --recurse-submodules https://github.com/dragonblocks/dragonblocks_alpha.git
```

## Dependencies
To build anything you need CMake. The ZLib development library is needed as well.

```bash
sudo apt install build-essential cmake zlib1g-dev
```

The development versions of OpenGL, GLFW3, GLEW and Freetype are required to build the client.

```bash
sudo apt install libgl1-mesa-dev libglfw3-dev libglew-dev libfreetype-dev
```

For building the server the SQLite3 development library is required.

```bash
sudo apt install libsqlite3-dev
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
