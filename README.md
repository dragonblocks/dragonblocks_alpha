# Dragonblocks Alpha

A multiplayer voxelgame for POSIX systems.

Head to <https://www.elidragon.tk/dragonblocks_alpha/> for ubuntu snapshot and release builds.

## Invocation

```bash
./DragonblocksServer <port>
./Dragonblocks <address> <port>
```

or alternatively:

```bash
./singleplayer.sh
```

## Controls

| Key | Action |
|-|-|
| W | Move Forward |
| A | Move Left |
| S | Move Backward |
| D | Move Right |
| Space | Jump / When flying: Move Up |
| Left Shift | When flying: Move Down |
| F | Toggle flight |
| C | Toggle collision |
| T | Toggle timelapse |
| F2 | Take screenshot |
| F3 | Toggle debug info |
| F11 | Toggle fullscreen |
| ESC | Pause / unpause game |

## Dependencies

Both the client and the server depend on ZLib.
The client depends on GLFW3, OpenGL, GLEW and Freetype.
The server depends on SQLite3.

Ubuntu / Debian:

```bash
sudo apt install zlib1g libgl1-mesa-dri libglfw3 libglew2.1 libfreetype6 libsqlite3-0

```

FreeBSD:

```csh
sudo pkg install lzlib mesa-dri glfw glew freetype sqlite3
```

OpenBSD:

```sh
sudo pkg_add lzlib glfw glew freetype sqlite3
```

## Setup Requirements
Dragonblocks Alpha targets PCs only. Non x86-64 platforms may work, however there is no guarantee whatsoever.
You need a POSIX system conforming to the ISO C and POSIX 2008 standards. However, so far only GNU/Linux systems, in particular Ubuntu and Debian, have been tested.
The minimum OpenGL version is 4.6.0. Dragonblocks has been tested on Intel Integrated and NVIDIA GPUs, however other graphics cards should work as well.
A PC with at least 4 CPU cores is recommended, but not necessarily required.

## Current Features
- Multiplayer, Animated player model, Nametags
- Mountains, snow, temperature and humidity, dynamic grass color, oceans and beaches, vulcanos, boulders
- Physics
- FPS Camera
- Mipmapping, Antialiasing, Face Culling, Frustum Culling, Diffuse Lighting, Skybox, Fog
- Taking screenshots
- Daylight cycle
- GUI
- Saving terrain, player positions and other data to a SQLite3 database
- Multithreaded terrain generation, mesh generation and networking
- Handlers for SIGINT und SIGTERM (just Ctrl+C to shut down the server)
- Log levels: error, warning, access, action, info, verbose
- Loading assets such as textures, models, schematics, shaders and fonts from files

## Usage

### Server
The server currently stores the world database (world.sqlite) in the current working directory, and it will stay like that.
If you want to have multiple worlds, just run the DragonblocksServer process from different directories.
It's up to you how you organize the world folders, which is an advantage since the program really just "does one thing well"
without having to search your system for share directories or maintaining a world list (like Minetest does).

### Client / Singleplayer
The Dragonblocks client itself does not and will not have a Main menu. It goes against the already mentioned UNIX philosophy to have a binary
that does multiple things at once. For now, there is a singleplayer script that launches a server and a client, and in the future a launcher
will be added that is used to do all the stuff that users nowadays don't want to do themselves, like showing you a list of worlds and launching the
server in the correct directory, as well as updating the game and managing game versions for you. It's gonna do the ugly `~/.program_name`, but you
wont't have to use it if you don't want to.

### Modding
Dragonblocks Alpha does not and will most likely never have a modding API. If anything, a Lua plugin API will be added.
It would be possible to have a native modding API for a C project (as demonstrated by [dungeon_game](https://github.com/EliasFleckenstein03/dungeon_game)),
but it would remove simplicity and, most importantly, remove optimisation possibilities.
The way you are meant to mod dragonblocks is by simply forking it on github and modifiying the game directly. To use multiple mods together, just git merge them.
If there are conflicts, the mods would likely not be compatible anyway.

## Project Goals
The name "Dragonblocks _Alpha_" does not have anything to do with the game being in early development (which it is tho), it's just the game's name.

### What Dragonblocks Alpha aims to achieve
- A voxelgame inspired by Minecraft and Veloren, with the techical side being inspired by Minetest
- Exciting and feature-rich gameplay with the focus on exploring and adventuring, while still being multi-optional and not too bloated
- A simple structure and invocation syntax
- Using modern OpenGL to combine performance with graphics quality on high-end computers
- Portability between PCs running POSIX systems (focus: Linux, BSD, MacOS, Plan 9 APE, Windows MinGW)

### What Dragonblocks Alpha does not aim to achieve
- Portability to Phones / Consoles
- Good performance on low-end PCs
- A fixed story or lore
- Cloning Minecraft behavior
- Replacement for Minecraft and / or Minetest
- An engine
