#!/bin/bash
PATH=".:$PATH"
dragonblocks_server "[::1]:4000" &
echo "singleplayer" | dragonblocks_client "[::1]:4000"
pkill -P $$
