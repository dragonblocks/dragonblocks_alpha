# Dragonblocks alpha

## Usage
```bash
./DragonblocksServer <port>
./Dragonblocks <address> <port>
```

The server will save the map to a file named "map" in the directory it is run in. It will also load the map from there at startup, given that the file exists.

## Client commands:
- `setnode <x> <y> <z> <node>`: set a node somewhere
- `getnode <x> <y> <z>`: download the mapblock containing the node at x y z from server
- `printnode <x> <y> <z>`: print the node at x y z (download the mapblock using getnode first)
- `kick <name>`: kick a player
- `disconnect`: disconnect from server

All positions are signed integers (`s32`)

## Nodes:
- `unloaded`: used for nodes in unloaded mapblocks
- `air`: default node in newly generated mapblocks
- `dirt`
- `grass`
- `stone`
- `invalid`: unknown nodes received from server

Interrupt handlers for SIGINT und SIGTERM are implemented.
