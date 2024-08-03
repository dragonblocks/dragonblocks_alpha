#!/bin/bash
PATH=".:$PATH"
dragonblocks-server "[::1]:4000" &
echo "singleplayer" | dragonblocks-client "[::1]:4000"
pkill -P $$
