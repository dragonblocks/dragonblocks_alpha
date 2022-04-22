#!/bin/bash
./dragonblocks_server "[::1]:4000" &
echo "singleplayer" | ./dragonblocks "[::1]:4000"
pkill -P $$ -9
