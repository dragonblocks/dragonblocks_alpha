# Dragonblocks alpha

A multiplayer voxelgame for POSIX systems.

## Usage

```bash
./DragonblocksServer <port>
./Dragonblocks <address> <port>
```

or alternatively:

```bash
./singleplayer.sh
```

The server will save the map to a file named "map" in the directory it is run in. It will also load the map from there at startup, given that the file exists.

Interrupt handlers for SIGINT und SIGTERM are implemented.

## Controls

Use W, A, S and D to move forward / left / backward / right.

## Dependencies

The client depends on GLFW3, OpenGL and GLEW.

```bash
sudo apt install libgl1-mesa-dri libglfw3 libglew2.1
```

The server depends on SQLite3.

```bash
sudo apt install libsqlite3-0
```

both the client and the server depend on ZLib.

```bash
sudo apt install zlib1g
```
