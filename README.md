# Dragonblocks Alpha

A multiplayer voxelgame for POSIX systems.
It has been ported to Linux, Windows, FreeBSD and OpenBSD, but may work on other systems.

Head to <https://dragonblocks.lizzy.rs> for snapshot and release builds.

## Invocation

```sh
# on posix
./dragonblocks-server "<address>:<port>"
./dragonblocks-client "<player name>" "<address>:<port>"

# on windows
dragonblocks-server.exe "<address>:<port>"
dragonblocks-client.exe "<player name>" "<address>:<port>"
```

or alternatively:

```sh
# on posix
./dragonblocks.sh singleplayer singleplayer_world

# on windows
singleplayer.bat
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
| I | Open / close inventory |
| F | Toggle flight |
| C | Toggle collision |
| T | Toggle timelapse |
| F2 | Take screenshot |
| F3 | Toggle debug info |
| F11 | Toggle fullscreen |
| ESC | Pause / unpause game |

## System Requirements
You need a system conforming to the C23 and POSIX 2024 standards. Other systems may be supported as well.
The minimum OpenGL version is 3.3 core.
A PC with at least 4 CPU cores is recommended, but not necessarily required.

## Current Features
- Multiplayer, Animated player model, Nametags
- Mountains, snow, temperature and humidity, dynamic grass color, oceans and beaches, vulcanos, boulders
- Physics
- Mipmapping, Antialiasing, Face Culling, Frustum Culling, Diffuse Lighting, Skybox, Fog
- Taking screenshots
- Daylight cycle
- Saving terrain, player positions and other data to a SQLite3 database
- Multithreaded terrain generation, mesh generation and networking
- Handlers for SIGINT und SIGTERM (just Ctrl+C to shut down the server)
- Log levels: error, warning, access, action, info, verbose
- Loading assets such as textures, models, schematics, shaders and fonts from files
- Inventory
- Tools, Breaking blocks

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
It would be possible to have a native modding API for a C project (as demonstrated by [dungeon_game](https://github.com/LizzyFleckenstein03/dungeon_game)),
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
- Portability between PCs running POSIX systems (focus: Linux, BSD, MacOS, Windows MinGW)

### What Dragonblocks Alpha does not aim to achieve
- Portability to Phones / Consoles
- Good performance on low-end PCs
- A fixed story or lore
- Cloning Minecraft behavior
- Replacement for Minecraft and / or Minetest
- An engine
