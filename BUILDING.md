# Building instructions

The code and the Makefile are located in the src/ directory

## Available targets
- `all` (default)
- `Dragonblocks`
- `DragonblocksServer`
- `clean`
- `clobber`

Debug flag (`-g`) is set by default.

## Release Build
```bash
make clobber && make all RELEASE=TRUE -j$(nproc) && make clean
```
