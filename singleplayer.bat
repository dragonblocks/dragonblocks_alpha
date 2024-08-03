@echo off
start "Internal Server" dragonblocks-server.exe "[::1]:4000"
echo "singleplayer" | dragonblocks-client.exe "[::1]:4000"
taskkill /FI "Internal Server" /T /F
