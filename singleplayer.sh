#! /bin/bash
./DragonblocksServer "[::1]:4000" & echo "singleplayer" | ./Dragonblocks "[::1]:4000"; killall DragonblocksServer
