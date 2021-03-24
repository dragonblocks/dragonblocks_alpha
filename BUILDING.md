# Building instructions

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
