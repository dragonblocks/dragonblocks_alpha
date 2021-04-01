# Building instructions

GNU make is used for compiling. The code and the Makefile are located in the src/ directory.

## Dependencies
To build anything you need g++ and GNU make.

```bash
sudo apt install build-essential make
```

The development versions OpenGL, GLFW3, GLEW are required to build the client.

```bash
sudo apt install libgl1-mesa-dev libglfw3-dev libglew-dev
```

For building the server, the SQLite3 development library is required.

```bash
sudo apt install libsqlite3-dev
```

Don't forget to pull the submodules before building.

``bash
git submodule update --init
```

## Available targets
- `all` (default)
- `Dragonblocks`
- `DragonblocksServer`
- `clean`
- `clobber`

The debug flag (`-g`) is set by default (RELEASE=TRUE will disable it).

## Release

```bash
./snapshot.sh
```
This script will create a snapshot zipfile.
